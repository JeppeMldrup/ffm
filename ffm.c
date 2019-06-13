#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#define namelength 50
#define width 50
#define token 30
#define hideDotFiles true

int h, w;
bool resized = false;

void resize(int sig);
void displayDir(DIR *cdir, int w, int cursor);

int main(){
        int cursor = 1, count = 0, iter = 0, color = 0;
        char ch;
        char buf[200], prevmode[4], wd[100], chbuf[200];
        FILE *cat;
        DIR *cdir, *prevdir, *nextdir;
        struct dirent *selection, *next;

        getcwd(wd, sizeof(wd));
        strcat(wd, "/");

        initscr();
        noecho();
        start_color();

        attron(A_BOLD);

        init_pair(0, COLOR_WHITE, COLOR_BLACK);
        init_pair(1, COLOR_RED, COLOR_BLACK);
        init_pair(2, COLOR_CYAN, COLOR_BLACK);

        curs_set(0);
        getmaxyx(stdscr, h, w);

        while(1){
                count = 0;
                iter = 0;

                addstr(wd);

                cdir = opendir(wd);
                
                count = 0;
                do{
                        selection = readdir(cdir);
                        if(selection->d_name[0] == '.' && hideDotFiles)
                                continue;
                        count++;
                }while(count <= cursor);

                cdir = opendir(wd);
                
                strcpy(buf, wd);
                strcat(buf, "/..");

                prevdir = opendir(buf);

                if(selection->d_type == DT_DIR)
                        strcpy(prevmode, "dir");
                else if(selection->d_type == DT_REG)
                        strcpy(prevmode, "cat");

                displayDir(cdir, width, cursor);
                displayDir(prevdir, 0, -1);

                iter = 1;
                count = width*2;
                if(strcmp(prevmode, "cat") == 0){
                        strcpy(buf, wd);
                        strcat(buf, selection->d_name);
                        cat = fopen(buf, "r");
                        while((ch = fgetc(cat)) != EOF){
                                if(iter >= h-1)
                                        break;
                                if(ch == '\n' || count >= w-1){
                                        iter++;
                                        count = width*2;
                                        continue;
                                }
                                move(iter, count);
                                addch(ch);
                                count++;
                        }
                }
                else if(strcmp(prevmode, "dir") == 0){
                        strcpy(buf, wd);
                        strcat(buf, selection->d_name);
                        strcat(buf, "/");
                        nextdir = opendir(buf);
                        displayDir(nextdir, width*2, -1);
                }

                char input = getch();
                switch(input){
                        case 'q':
                                endwin();
                                exit(0);
                        case 66:
                        case 'n':
                                cursor++;
                                move(0, 0);
                                closedir(cdir);
                                closedir(prevdir);
                                clrtobot();
                                continue;
                        case 65:
                        case 'e':
                                cursor--;
                                move(0, 0);
                                closedir(cdir);
                                closedir(prevdir);
                                clrtobot();
                                continue;
                        case 'u':
                                getmaxyx(stdscr, h, w);
                                move(0, 0);
                                closedir(cdir);
                                closedir(prevdir);
                                clrtobot();
                                continue;
                        case 67:
                        case 'i':
                                if(strcmp(prevmode, "dir") != 0){
                                        strcat(wd, selection->d_name);
                                        snprintf(buf, sizeof(buf), "xdg-open %s", wd);
                                        system(buf);
                                        endwin();
                                        exit(0);
                                }
                                else{
                                        iter = 0;
                                        strcat(wd, selection->d_name);
                                        strcat(wd, "/");
                                        move(0, 0);
                                        closedir(cdir);
                                        closedir(prevdir);
                                        clrtobot();
                                        cursor = 0;
                                        selection = NULL;
                                        continue;
                                }
                        case 68:
                        case 'h':
                                count = 0;
                                for(iter = 0; iter < 100; iter++){
                                        if(wd[iter] == '\0'){
                                                count = iter-2;
                                                while(wd[count] != '/' && count > 0){
                                                        wd[count] = '\0';
                                                        count--;
                                                }
                                                move(0, 0);
                                                closedir(cdir);
                                                closedir(prevdir);
                                                clrtobot();
                                                cursor = 0;
                                                selection = NULL;
                                                break;
                                        }
                                }
                                continue;
                        default:
                                move(0, 0);
                                closedir(cdir);
                                closedir(prevdir);
                                clrtobot();
                                continue;
                }
        }
        return 0;
}

void displayDir(DIR *cdir, int w, int cursor){
        int count = 0, i = 0;
        int color;
        char buf[100], ch;
        struct dirent *dir;
        
        while((dir = readdir(cdir)) != NULL){
                if(dir->d_name[0] == '.' && hideDotFiles)
                        continue;
                move(count+1, w);

                if(cursor == count){
                        if(dir->d_type == DT_REG){
                                while(dir->d_name[i] != '\0'){
                                        addch(dir->d_name[i] | COLOR_PAIR(1) | A_REVERSE);
                                        i++;
                                }
                        }
                        else if(dir->d_type == DT_DIR){
                                while(dir->d_name[i] != '\0'){
                                        addch(dir->d_name[i] | COLOR_PAIR(2) | A_REVERSE);
                                        i++;
                                }
                        }
                        else{
                                while(dir->d_name[i] != '\0'){
                                        addch(dir->d_name[i] | A_REVERSE);
                                        i++;
                                }
                        }
                        i = 0;
                }
                else{
                        if(dir->d_type == DT_REG){
                                while(dir->d_name[i] != '\0'){
                                        addch(dir->d_name[i] | COLOR_PAIR(1));
                                        i++;
                                }
                        }
                        else if(dir->d_type == DT_DIR){
                                while(dir->d_name[i] != '\0'){
                                        addch(dir->d_name[i] | COLOR_PAIR(2));
                                        i++;
                                }
                        }
                        else{
                                while(dir->d_name[i] != '\0'){
                                        addch(dir->d_name[i]);
                                        i++;
                                }
                        }
                        i = 0;
                }
                count++;
        }
}
