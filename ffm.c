#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#define bufsize 1024
#define namelength 50
#define dirsize 100
#define width 50
#define token 30

volatile bool hideDotFiles = true;

int h, w;
bool resized = false; 

void resize(int sig);
void displayDir(struct dirent *dir[], int w, int cursor, int size, int h);
void sortDir(struct dirent *dir[], int size);
void prevDir(char *wd);
void sendMessage(char *message, bool show);

int main(){
        int cursor = 0, count, i, j, match[dirsize], matchcursor = 0, matchcount;
        char ch, input;
        char buf[bufsize], prevmode[4], wd[bufsize], chbuf[bufsize], cploc[100], mvloc[100], cpdest[100], message[100], locate[100];
        FILE *cat = NULL;
        DIR *cdir = NULL, *prevdir = NULL, *nextdir = NULL;
        struct dirent *prev[dirsize], *selection[dirsize], *next[dirsize];
        bool isDir, typing = false, searching = false;

        cploc[0] = mvloc[0] = message[0] = locate[0] = '\0';

        getcwd(wd, sizeof(wd));
        strcat(wd, "/");

        initscr();
        noecho();
        start_color();
	keypad(stdscr, true);

        attron(A_BOLD);

        use_default_colors();

        init_pair(0, COLOR_WHITE, -1);
        init_pair(1, COLOR_RED, -1);
        init_pair(2, COLOR_CYAN, -1);

        curs_set(0);
        getmaxyx(stdscr, h, w);

        signal(SIGWINCH, resize);

        while(1){
                count = 0;
                i = 0;
                j = 0;

                move(0, 0);
                addstr(wd);

		cdir = opendir(wd);

                while(1){
			if(cdir == NULL)
				break;
                        if((selection[count] = readdir(cdir)) == NULL || count >= dirsize)
                                break;
                        if(selection[count]->d_name[0] == '.' && hideDotFiles)
                                continue;
                        count++;
                }

                if(cursor < 0)
                        cursor = count-1;
                else if(cursor >= count)
                        cursor = 0;
                
                if(selection[0] != NULL){
                        sortDir(selection, count);

                        rewinddir(cdir);
                        
                        if(selection[cursor]->d_type == DT_DIR)
                                strcpy(prevmode, "dir");
                        else if(selection[cursor]->d_type == DT_REG)
                                strcpy(prevmode, "cat");

                        displayDir(selection, (int)w/4, cursor, count, h);
                }
                else
                        strcpy(prevmode, "mpt");

		strcpy(buf, wd);
		strcat(buf, "..");
                if(wd[1] != '\0' && (prevdir = opendir(buf)) != NULL){
                        count = 0;
                        while(1){
                                if((prev[count] = readdir(prevdir)) == NULL || count >= dirsize)
                                        break;
                                if(prev[count]->d_name[0] == '.' && hideDotFiles)
                                        continue;
                                count++;
                        }
                        if(prev[0] != NULL){
                                sortDir(prev, count);
                                displayDir(prev, 0, -1, count, h);
                        }
			closedir(prevdir);
                }

                i = 1;
                j = (int)w/2;
		strcpy(buf, wd);
		strcat(buf, selection[cursor]->d_name);
		if(access(buf, R_OK) < 0){
			strcpy(message, "File is not readable");
			sendMessage(message, !searching);
			strcpy(prevmode, "mpt");
		}
		else if(strcmp(prevmode, "cat") == 0){
			if((cat = fopen(buf, "r")) == NULL)
				break;
                        while((ch = fgetc(cat)) != EOF){
                                if(i >= h-1)
                                        break;
                                if(ch == '\n' || j >= w-1){
                                        i++;
                                        j = (int)w/2;
                                        continue;
                                }
                                move(i, j);
                                addch(ch);
                                j++;
                        }
                }
                else if(strcmp(prevmode, "dir") == 0){
			if((nextdir = opendir(buf)) == NULL)
				break;
                        count = 0;
                        while(1){
                                if((next[count] = readdir(nextdir)) == NULL || count >= dirsize)
                                        break;
                                if(next[count]->d_name[0] == '.' && hideDotFiles)
                                        continue;
                                count++;
                        }
                        if(next[0] != NULL){
                                sortDir(next, count);
                                displayDir(next, (int)w/2, -1, count, h);
                        }
			else
				strcpy(prevmode, "mpt");
			closedir(nextdir);
                }

		if(cdir != NULL)
			closedir(cdir);

		if(typing == true){
			count = 0;
			i = 0;
			while(1){
				if(selection[count] == NULL)
					break;
				else if(strstr(selection[count]->d_name, locate) != NULL){
					size_t size = strcspn(selection[count]->d_name, locate);
					move(count+1, (int)w/4+(int)size);
					attron(A_REVERSE);
					addstr(locate);
					attroff(A_REVERSE);
					match[i] = count;
					i++;
				}
				count++;
			}
			matchcount = i;
			snprintf(message, sizeof(message), "%i matches found", i);
			sendMessage(message, !searching);

			ch = getch();
			if(ch == (char)KEY_BACKSPACE)
				locate[strlen(locate)-1] = '\0';
			else if(ch == 10){
				cursor = match[0];
				typing = false;
				searching = true;
				locate[0] = '\0';
				erase();
				snprintf(message, sizeof(message), "1 of %i", matchcount);
				sendMessage(message, true);
				continue;
			}
			else if(ch >= 32 && ch <= 126)
				strcat(locate, &ch);
			else{
				typing = false;
				locate[0] = '\0';
				continue;
			}

			erase();
			move(h-1, (int)w/2);
			addstr(locate);
			continue;
		}

                input = getch();
                switch(input){
			case 'n':
				if(searching){
					matchcursor++;
					if(matchcursor >= matchcount)
						matchcursor = 0;
					cursor = match[matchcursor];
					snprintf(message, sizeof(message), "%i of %i", matchcursor+1, matchcount);
					erase();
					sendMessage(message, true);
					continue;
				}
				erase();
				continue;
                        case 'q':
                                endwin();
                                exit(0);
                        case (char)KEY_DOWN:
                        case 'j':
                                cursor++;
                                erase();
                                continue;
                        case (char)KEY_UP:
                        case 'k':
                                cursor--;
                                erase();
                                continue;
                        case 'u':
                                getmaxyx(stdscr, h, w);
                                erase();
                                continue;
                        case (char)KEY_RIGHT:
                        case 'l':
				searching = false;
                                if(strcmp(prevmode, "mpt") == 0){
                                        strcpy(message, "Folder is empty");
					sendMessage(message, true);
                                        continue;
                                }
                                else if(strcmp(prevmode, "dir") != 0){
                                        strcat(wd, selection[cursor]->d_name);
                                        snprintf(buf, bufsize, "xdg-open %s", wd);
                                        system(buf);
                                        endwin();
                                        exit(0);
                                }
                                else{
                                        strcat(wd, selection[cursor]->d_name);
                                        strcat(wd, "/");
                                        erase();
                                        selection[0] = NULL;
                                        continue;
                                }
                        case (char)KEY_LEFT:
                        case 'h':
				searching = false;
                                j = 0;
				if(wd[1] == '\0'){
					strcpy(message, "At root directory\0");
					sendMessage(message, true);
					continue;
				}
				prevDir(wd);
				selection[0] = NULL;
                                continue;
                        case 'y':
                                if(selection[0] == NULL){
                                        strcpy(message, "No file selected");
					sendMessage(message, true);
                                        continue;
                                }
                                strcpy(cploc, wd);
                                strcat(cploc, selection[cursor]->d_name);
                                if(selection[cursor]->d_type == DT_DIR)
                                        isDir = true;
                                else
                                        isDir = false;
                                mvloc[0] = '\0';
                                erase();
                                snprintf(message, sizeof(message), "File %s yanked for copying", selection[cursor]->d_name);
				sendMessage(message, true);
                                continue;
                        case 'm':
                                if(selection[0] == NULL){
                                        strcpy(message, "No file selected");
					sendMessage(message, true);
                                        continue;
                                }
                                strcpy(mvloc, wd);
                                strcat(mvloc, selection[cursor]->d_name);
                                cploc[0] = '\0';
                                erase();
                                snprintf(message, sizeof(message), "File %s yanked for moving", selection[cursor]->d_name);
				sendMessage(message, true);
                                continue;
                        case 'p':
                                if(cploc[0] != '\0'){
                                        if(isDir)
                                                snprintf(buf, bufsize, "cp -r %s %s", cploc, wd);
                                        else
                                                snprintf(buf, bufsize, "cp %s %s", cploc, wd);
                                        system(buf);
                                        cploc[0] = '\0';
                                        erase();
                                        continue;
                                }
                                else if(mvloc[0] != '\0'){
                                        snprintf(buf, bufsize, "mv %s %s", mvloc, wd);
                                        system(buf);
                                        mvloc[0] = '\0';
                                        erase();
                                        continue;
                                }
                                else{
                                        erase();
                                        strcpy(message, "No file selected");
					sendMessage(message, true);
                                        continue;
                                }
			case '.':
				hideDotFiles = !hideDotFiles;
				erase();
				continue;
			case 'g':
				input = getch();
				if(input == 'g')
					cursor = 0;
				erase();
				continue;
			case 'G':
				for(i = 0; i < bufsize; i++){
					if(selection[i] == NULL){
						cursor = i-1;
						break;
					}
				}
				erase();
				continue;
			case '/':
				typing = true;
				erase();
				continue;
                        default:
                                erase();
                                continue;
                }
        }

        exit(0);
}

void displayDir(struct dirent *dir[], int w, int cursor, int size, int h){
        int j, i, color;
        int offset = cursor - (h)/2 - 1;

        if(offset < 0 || size < (h-2))
                offset = 0;

        for(i = offset; i < size; i++){
                j = 0;
                move(i-offset+1, w);

                if(dir[i] == NULL || i-offset+1 > h-1)
                        break;
                if(cursor == i){
                        switch(dir[i]->d_type){
				case DT_REG:
					attron(COLOR_PAIR(1) | A_REVERSE);
					addstr(dir[i]->d_name);
					attroff(COLOR_PAIR(1) | A_REVERSE);
					continue;
				case DT_DIR:
					attron(COLOR_PAIR(2) | A_REVERSE);
					addstr(dir[i]->d_name);
					addch('/');
					attroff(COLOR_PAIR(2) | A_REVERSE);
					continue;
				default:
					attron(A_REVERSE);
					addstr(dir[i]->d_name);
					attroff(A_REVERSE);
			}
		}
                else{
                        switch(dir[i]->d_type){
				case DT_REG:
					attron(COLOR_PAIR(1));
					addstr(dir[i]->d_name);
					attroff(COLOR_PAIR(1));
					continue;
				case DT_DIR:
					attron(COLOR_PAIR(2));
					addstr(dir[i]->d_name);
					addch('/');
					attroff(COLOR_PAIR(2));
					continue;
				default:
					addstr(dir[i]->d_name);
			}
                }
        }
}

void sortDir(struct dirent *dir[], int size){
        struct dirent *buf[size];
        int i = 0, j = 0;

        for(i; i < size; i++){
                if(dir[i]->d_type == DT_DIR){
                        buf[j] = dir[i];
                        j++;
                }
        }

        for(i = 0; i < size; i++){
                if(dir[i]->d_type != DT_DIR){
                        buf[j] = dir[i];
                        j++;
                }
        }

        for(i = 0; i < size; i++){
                dir[i] = buf[i];
        }
}

void resize(int sig){
//        endwin();
//
//        refresh();
//        clear();
}

void prevDir(char *wd){
	int i = 0, j = 0;
	for(i; i < bufsize; i++){
		if(wd[i] == '\0'){
			j = i-2;
			while(wd[j] != '/' && j > 0){
				wd[j] = '\0';
				j--;
			}
			erase();
			break;
		}
	}
}

void sendMessage(char *message, bool show){
	if(!show)
		return;
	move(h-1, 0);
	addstr(message);
}
