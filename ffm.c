#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <signal.h>
#include <unistd.h>

#define namelength 50
#define width 50
#define token 30

int h, w;
bool resized = false;

void resize(int sig);
void displayDir(FILE *fp, int w, int cursor);

int main(){
        int cursor = 1, count = 0, iter = 0, color = 0;
        char ch;
        char selection[namelength], buf[200], filetype[10], prevmode[4], wd[100], chbuf[200];
        FILE *fpwd, *fpselection, *fpprev;

        if(system("ls -1SX --file-type > /tmp/dir.txt") != 0){
                printf("Could not get current directory\n");
                exit(0);
        }

        getcwd(wd, sizeof(wd));
        strcat(wd, "/");

        system("ls -1SX --file-type ../ > /tmp/prevdir.txt");

        fpwd = fopen("/tmp/dir.txt", "r");
        fpprev = fopen("/tmp/prevdir.txt", "r");

        if (fpwd == NULL){
                printf("Could not read /tmp/dir.txt\n");
                exit(0);
        }

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
                filetype[0] = '\0';

                while((ch = fgetc(fpwd)) != EOF){  //Find the selection
                        if(ch == '\n'){
                                count++;
                                if(count > cursor){
                                        selection[iter] = '\0';
                                        break;
                                }
                                iter = 0;
                                continue;
                        }
                        selection[iter] = ch;
                        iter++;
                }


                iter = 0;
                while(selection[iter] != '\0'){  //Get the fileextension of the selection
                        if(selection[iter] == '.'){
                                for(count = 0; count < 10; count++){
                                        filetype[count] = selection[iter+count+1];
                                        if(selection[iter+count+1] == '\0')
                                                break;
                                }
                        }
                        else if(selection[iter] == '/'){
                                strcpy(chbuf, wd);
                                strcat(chbuf, selection);
                                snprintf(buf, sizeof(buf), "ls -1SX --file-type %s > /tmp/lst.txt", chbuf);
                                system(buf);
                                fpselection = fopen("/tmp/lst.txt", "r");
                                strcpy(prevmode, "lst");
                        }
                        iter++;
                }

                if(filetype != "png" && filetype != "jpg" && filetype[0] != '\0'){  //If file has extension and not a picture, cat it to cat.txt
                        strcpy(chbuf, wd);
                        strcat(chbuf, selection);        
                        if(snprintf(buf, sizeof(buf), "cat %s > /tmp/cat.txt", chbuf) > 0){
                                        fpselection = fopen("/tmp/cat.txt", "r");
                                        strcpy(prevmode, "cat");
                        
                        }        
                        system(buf);
                }

                fpwd = freopen("/tmp/dir.txt", "r", fpwd);

                displayDir(fpwd, width, cursor);
                
                snprintf(buf, sizeof(buf), "ls -1SX --file-type %s/.. > /tmp/prevdir.txt", wd);
                system(buf);

                fpprev = freopen("/tmp/prevdir.txt", "r", fpprev);
                if(fpprev != NULL)
                        displayDir(fpprev, 0, -1);

                move(0, 0);
                addstr(wd);

                iter = 0;
                count = 0;
                if(fpselection != NULL && strcmp(prevmode, "cat") == 0){
                        while((ch = fgetc(fpselection)) != EOF){
                                move(iter+1, 2*width+count);

                                if(ch == '\n'){
                                        iter++;
                                        if(iter >= h)
                                                break;
                                        count = 0;
                                        continue;
                                }
                                if(width+count < w)
                                        addch(ch);
                                count++;
                        }
                        iter = 0;
                        count = 0;
                }
                else if(fpselection != NULL && strcmp(prevmode, "lst") == 0){
                        displayDir(fpselection, width*2, -1);
                }

                //signal(SIGWINCH, resize);

                //if(resized){
                //        resized = false;
                //        continue;
                //}

                char input = getch();
                switch(input){
                        case 'q':
                                system("rm /tmp/dir.txt");
                                system("rm /tmp/cat.txt");
                                endwin();
                                exit(0);
                        case 'n':
                                cursor++;
                                fpwd = freopen("/tmp/dir.txt", "r", fpwd);
                                move(0, 0);
                                clrtobot();
                                continue;
                        case 'e':
                                cursor--;
                                fpwd = freopen("/tmp/dir.txt", "r", fpwd);
                                move(0, 0);
                                clrtobot();
                                continue;
                        case 'u':
                                getmaxyx(stdscr, h, w);
                                fpwd = freopen("/tmp/dir.txt", "r", fpwd);
                                move(0, 0);
                                clrtobot();
                                continue;
                        case 'i':
                                if(strcmp(prevmode, "lst") != 0){
                                        strcat(wd, selection);
                                        snprintf(buf, sizeof(buf), "xdg-open %s", wd);
                                        system(buf);
                                        endwin();
                                        exit(0);
                                }
                                else{
                                        iter = 0;
                                        strcat(wd, selection);
                                        snprintf(buf, sizeof(buf), "ls -1SX --file-type %s > /tmp/dir.txt", wd);
                                        system(buf);
                                        fpwd = freopen("/tmp/dir.txt", "r", fpwd);
                                        move(0, 0);
                                        clrtobot();
                                        cursor = 0;
                                        selection[0] = '\0';
                                        continue;
                                }
                        case 'h':
                                count = 0;
                                for(iter = 0; iter < 100; iter++){
                                        if(wd[iter] == '\0'){
                                                count = iter-2;
                                                while(wd[count] != '/' && count > 0){
                                                        wd[count] = '\0';
                                                        count--;
                                                }
                                                snprintf(buf, sizeof(buf), "ls -1SX --file-type %s > /tmp/dir.txt", wd);
                                                system(buf);
                                                fpwd = freopen("/tmp/dir.txt", "r", fpwd);
                                                move(0, 0);
                                                clrtobot();
                                                cursor = 0;
                                                selection[0] = '\0';
                                                break;
                                        }
                                }
                                continue;
                        default:
                                fpwd = freopen("/tmp/dir.txt", "r", fpwd);
                                move(0, 0);
                                clrtobot();
                                continue;
                }
        }
        return 0;
}

void displayDir(FILE *fp, int w, int cursor){
        int iter = 0, count = 0;
        int color;
        char buf[100], ch;
        
        while((ch = fgetc(fp)) != EOF){
                move(iter+1, w);
                if(ch != '\n'){
                        buf[count] = ch;
                        count++;
                }
                else{
                        buf[count] = '\0';
                        count = 0;
                        while(buf[count] != '\0'){
                                if(buf[count] == '.'){
                                        color = 1;
                                        break;
                                }
                                else if(buf[count] == '/'){
                                        color = 2;
                                        break;
                                }
                                count++;
                        }
                        count = 0;
                        if(iter == cursor){
                                while(buf[count] != '\0'){
                                        if(strlen(buf) >= token){
                                                if(count >= token)
                                                        break;
                                                else if(count >= token-3)
                                                        buf[count] = '.';
                                        }
                                        switch(color){
                                                case 1:
                                                        addch(buf[count] | COLOR_PAIR(1) | A_REVERSE);
                                                        break;
                                                case 2:
                                                        addch(buf[count] | COLOR_PAIR(2) | A_REVERSE);
                                                        break;
                                                default:
                                                        addch(buf[count] | COLOR_PAIR(0) | A_REVERSE);
                                                        break;
                                        }
                                        count++;
                                }
                                switch(color){
                                        case 2:
                                                move(iter+1, w+token);
                                                addch('/' | COLOR_PAIR(2));
                                                break;
                                        case 1:
                                                move(iter+1, w+token);
                                                addch('$' | COLOR_PAIR(1));
                                                break;
                                        default:
                                                move(iter+1, w+token);
                                                addch(ACS_VLINE | COLOR_PAIR(0));
                                                break;
                                }
                                count = 0;
                                color = 0;
                        }else{
                                while(buf[count] != '\0'){
                                        if(strlen(buf) >= token){
                                                if(count >= token)
                                                        break;
                                                else if(count >= token-3)
                                                        buf[count] = '.';
                                        }
                                        switch(color){
                                                case 1:
                                                        addch(buf[count] | COLOR_PAIR(1));
                                                        break;
                                                case 2:
                                                        addch(buf[count] | COLOR_PAIR(2));
                                                        break;
                                                default:
                                                        addch(buf[count] | COLOR_PAIR(0));
                                                        break;
                                        }
                                        count++;
                                }
                                switch(color){
                                        case 2:
                                                move(iter+1, w+token);
                                                addch('/' | COLOR_PAIR(2));
                                                break;
                                        case 1:
                                                move(iter+1, w+token);
                                                addch('$' | COLOR_PAIR(1));
                                                break;
                                        default:
                                                move(iter+1, w+token);
                                                addch(ACS_VLINE | COLOR_PAIR(0));
                                                break;
                                }
                                count = 0;
                                color = 0;
                        }
                        iter++;
                }
        }
}
