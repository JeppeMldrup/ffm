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

int main(){
        int cursor = 0, count, i, j;
        char ch;
        char buf[bufsize], prevmode[4], wd[bufsize], chbuf[bufsize], cploc[100], mvloc[100], cpdest[100], message[100];
        FILE *cat = NULL;
        DIR *cdir = NULL, *prevdir = NULL, *nextdir = NULL;
        struct dirent *prev[dirsize], *selection[dirsize], *next[dirsize];
        bool isDir;

        cploc[0] = mvloc[0] = message[0] = '\0';

        getcwd(wd, sizeof(wd));
        strcat(wd, "/");

        initscr();
        noecho();
        start_color();

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
                j = width*2;
		strcpy(buf, wd);
		strcat(buf, selection[cursor]->d_name);
		if(access(buf, R_OK) < 0){
			strcpy(message, "File is not readable");
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

                move(h-1, 0);
                addstr(message);

		if(cdir != NULL)
			closedir(cdir);

                char input = getch();
                switch(input){
                        case 'q':
                                endwin();
                                exit(0);
                        case 66:
                        case 'n':
                                cursor++;
                                erase();
                                continue;
                        case 65:
                        case 'e':
                                cursor--;
                                erase();
                                continue;
                        case 'u':
                                getmaxyx(stdscr, h, w);
                                erase();
                                continue;
                        case 67:
                        case 'i':
                                if(strcmp(prevmode, "mpt") == 0){
                                        strcpy(message, "Folder is empty");
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
                        case 68:
                        case 'h':
                                j = 0;
				if(wd[1] == '\0'){
					strcpy(message, "At root directory\0");
					continue;
				}
				prevDir(wd);
				selection[0] = NULL;
                                continue;
                        case 'y':
                                if(selection[0] == NULL){
                                        strcpy(message, "No file selected");
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
                                continue;
                        case 'm':
                                if(selection[0] == NULL){
                                        strcpy(message, "No file selected");
                                        continue;
                                }
                                strcpy(mvloc, wd);
                                strcat(mvloc, selection[cursor]->d_name);
                                cploc[0] = '\0';
                                erase();
                                snprintf(message, sizeof(message), "File %s yanked for moving", selection[cursor]->d_name);
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
                                        continue;
                                }
			case '.':
				hideDotFiles = !hideDotFiles;
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
        int j, i;
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
