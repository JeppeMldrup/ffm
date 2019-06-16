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
#define width 50
#define token 30
#define hideDotFiles true

int h, w;
bool resized = false;

void resize(int sig);
void displayDir(struct dirent *dir[], int w, int cursor, int size);
void sortDir(struct dirent *dir[], int size);

int main(){
        int cursor = 0, count, i, j;
        char ch;
        char buf[bufsize], prevmode[4], wd[100], chbuf[bufsize], cploc[100], mvloc[100], cpdest[100], message[100];
        FILE *cat;
        DIR *cdir, *prevdir, *nextdir;
        struct dirent *prev[40], *selection[40], *next[40];
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

                move(h-1, 0);
                addstr(message);

                move(0, 0);
                addstr(wd);

                cdir = opendir(wd);
                
                while(1){
                        if((selection[count] = readdir(cdir)) == NULL || count > 40)
                                break;
                        if(selection[count]->d_name[0] == '.' && hideDotFiles)
                                continue;
                        count++;
                }
                sortDir(selection, count);
                if(cursor < 0)
                        cursor = count-1;
                else if(cursor >= count)
                        cursor = 0;

                cdir = opendir(wd);
                
                if(selection[cursor]->d_type == DT_DIR)
                        strcpy(prevmode, "dir");
                else if(selection[cursor]->d_type == DT_REG)
                        strcpy(prevmode, "cat");

                displayDir(selection, width, cursor, count);

                if(wd[1] != '\0'){
                        strcpy(buf, wd);
                        strcat(buf, "/..");

                        prevdir = opendir(buf);
                        
                        count = 0;
                        while(1){
                                if((prev[count] = readdir(prevdir)) == NULL || count > 40)
                                        break;
                                if(prev[count]->d_name[0] == '.' && hideDotFiles)
                                        continue;
                                count++;
                        }

                        sortDir(prev, count);
                        displayDir(prev, 0, -1, count);
                }

                i = 1;
                j = width*2;
                if(strcmp(prevmode, "cat") == 0){
                        strcpy(buf, wd);
                        strcat(buf, selection[cursor]->d_name);
                        cat = fopen(buf, "r");
                        while((ch = fgetc(cat)) != EOF){
                                if(i >= h-1)
                                        break;
                                if(ch == '\n' || j >= w-1){
                                        i++;
                                        j = width*2;
                                        continue;
                                }
                                move(i, j);
                                addch(ch);
                                j++;
                        }
                }
                else if(strcmp(prevmode, "dir") == 0){
                        strcpy(buf, wd);
                        strcat(buf, selection[cursor]->d_name);
                        strcat(buf, "/");
                        nextdir = opendir(buf);
                        count = 0;
                        while(1){
                                if((next[count] = readdir(nextdir)) == NULL || count > 40)
                                        break;
                                if(next[count]->d_name[0] == '.' && hideDotFiles)
                                        continue;
                                count++;
                        }
                        sortDir(next, count);
                        displayDir(next, width*2, -1, count);
                }

                char input = getch();
                switch(input){
                        case 'q':
                                endwin();
                                exit(0);
                        case 66:
                        case 'n':
                                cursor++;
                                closedir(cdir);
                                closedir(prevdir);
                                erase();
                                continue;
                        case 65:
                        case 'e':
                                cursor--;
                                closedir(cdir);
                                closedir(prevdir);
                                erase();
                                continue;
                        case 'u':
                                getmaxyx(stdscr, h, w);
                                closedir(cdir);
                                closedir(prevdir);
                                erase();
                                continue;
                        case 67:
                        case 'i':
                                if(strcmp(prevmode, "dir") != 0){
                                        strcat(wd, selection[cursor]->d_name);
                                        snprintf(buf, sizeof(buf), "xdg-open %s", wd);
                                        system(buf);
                                        endwin();
                                        exit(0);
                                }
                                else{
                                        i = 0;
                                        strcat(wd, selection[cursor]->d_name);
                                        strcat(wd, "/");
                                        closedir(cdir);
                                        closedir(prevdir);
                                        erase();
                                        selection[0] = NULL;
                                        continue;
                                }
                        case 68:
                        case 'h':
                                j = 0;
                                for(i = 0; i < 100; i++){
                                        if(wd[i] == '\0'){
                                                j = i-2;
                                                while(wd[j] != '/' && j > 0){
                                                        wd[j] = '\0';
                                                        j--;
                                                }
                                                closedir(cdir);
                                                closedir(prevdir);
                                                erase();
                                                selection[0] = NULL;
                                                break;
                                        }
                                }
                                continue;
                        case 'y':
                                strcpy(cploc, wd);
                                strcat(cploc, selection[cursor]->d_name);
                                if(selection[cursor]->d_type == DT_DIR)
                                        isDir = true;
                                else
                                        isDir = false;
                                mvloc[0] = '\0';
                                closedir(cdir);
                                closedir(prevdir);
                                erase();
                                snprintf(message, sizeof(message), "File %s yanked for copying", selection[cursor]->d_name);
                                continue;
                        case 'm':
                                strcpy(mvloc, wd);
                                strcat(mvloc, selection[cursor]->d_name);
                                cploc[0] = '\0';
                                closedir(cdir);
                                closedir(prevdir);
                                erase();
                                snprintf(message, sizeof(message), "File %s yanked for moving", selection[cursor]->d_name);
                                continue;
                        case 'p':
                                if(cploc[0] != '\0'){
                                        if(isDir)
                                                snprintf(buf, sizeof(buf), "cp -r %s %s", cploc, wd);
                                        else
                                                snprintf(buf, sizeof(buf), "cp %s %s", cploc, wd);
                                        system(buf);
                                        cploc[0] = '\0';
                                        closedir(cdir);
                                        closedir(prevdir);
                                        erase();
                                        continue;
                                }
                                else if(mvloc[0] != '\0'){
                                        snprintf(buf, sizeof(buf), "mv %s %s", mvloc, wd);
                                        system(buf);
                                        mvloc[0] = '\0';
                                        closedir(cdir);
                                        closedir(prevdir);
                                        erase();
                                        continue;
                                }
                                else{
                                        closedir(cdir);
                                        closedir(prevdir);
                                        erase();
                                        strcpy(message, "No file selected");
                                        continue;
                                }
                        default:
                                closedir(cdir);
                                closedir(prevdir);
                                erase();
                                continue;
                }
        }
        return 0;
}

void displayDir(struct dirent *dir[], int w, int cursor, int size){
        int j = 0, i = 0;
        char buf[100], ch;
       
        for(i = 0; i < size; i++){
                move(i+1, w);

                if(cursor == i){
                        if(dir[i]->d_type == DT_REG){
                                while(dir[i]->d_name[j] != '\0'){
                                        addch(dir[i]->d_name[j] | COLOR_PAIR(1) | A_REVERSE);
                                        j++;
                                }
                        }
                        else if(dir[i]->d_type == DT_DIR){
                                while(dir[i]->d_name[j] != '\0'){
                                        addch(dir[i]->d_name[j] | COLOR_PAIR(2) | A_REVERSE);
                                        j++;
                                }
                                addch('/' | COLOR_PAIR(2) | A_REVERSE);
                        }
                        else{
                                while(dir[i]->d_name[j] != '\0'){
                                        addch(dir[i]->d_name[j] | A_REVERSE);
                                        j++;
                                }
                        }
                        j = 0;
                }
                else{
                        if(dir[i]->d_type == DT_REG){
                                while(dir[i]->d_name[j] != '\0'){
                                        addch(dir[i]->d_name[j] | COLOR_PAIR(1));
                                        j++;
                                }
                        }
                        else if(dir[i]->d_type == DT_DIR){
                                while(dir[i]->d_name[j] != '\0'){
                                        addch(dir[i]->d_name[j] | COLOR_PAIR(2));
                                        j++;
                                }
                                addch('/' | COLOR_PAIR(2));
                        }
                        else{
                                while(dir[i]->d_name[j] != '\0'){
                                        addch(dir[i]->d_name[j]);
                                        j++;
                                }
                        }
                        j = 0;
                }
        }
}

void sortDir(struct dirent *dir[], int size){
        struct dirent *buf[size];
        int i, j;

        for(i = 0; i < size; i++){
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
