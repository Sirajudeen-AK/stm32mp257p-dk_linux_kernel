#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdlib.h>

#define DELETE_NODE 1

#define fflush_input  int c;while ((c = getchar()) != '\n' && c != EOF);

bool validate_input(int *input) 
{
	// Check if scanf successfully read 1 integer
    if (scanf("%d", input) != 1) 
    {
        printf("Invalid input! Please enter a number.\n");
        
        // Flush the broken characters out of the input buffer (stdin)
       fflush_input
        
        return false; // Jump back to the top of the while loop and show menu again
    }
    return true;
}

int main()
{
    int fd;
    int choice;
    int id;
    char name[32];
    char buffer[128];
	int ret_ = 0;

    fd = open("/dev/list_demo", O_RDWR);

    if (fd < 0)
    {
        perror("open");
        return -1;
    }

    while (1)
    {
        printf("\n");
        printf("1. Add Student\n");
        printf("2. Show Students\n");
        printf("3. Delete Student\n");
        printf("4. Exit\n");

        printf("Choice : ");
		if (!validate_input(&choice))  continue; // Jump back to the top of the while loop and show menu again

        switch(choice)
        {
            case 1:

                printf("ID : ");
                if (!validate_input(&id)) continue;

                printf("Name : ");
                scanf("%31s",name);
				fflush_input

                snprintf(buffer,
                         sizeof(buffer),
                         "%d %s",
                         id,
                         name);

                ret_ = write(fd,
                      buffer,
                      strlen(buffer));

				if(ret_ == 0) printf("Write failed\n");
				else printf("Write successfully");

                break;

            case 2:

                memset(buffer,0,sizeof(buffer));

                read(fd,
                     buffer,
                     sizeof(buffer));

                printf("\n%s\n",buffer);

                lseek(fd,0,SEEK_SET);

                break;

            case 3:

                printf("Delete ID : ");
                if (!validate_input(&id)) continue;

				ret_= ioctl(fd, DELETE_NODE, id);

                if(ret_ == 2) printf("Invalid Control ID!\n");
                else if(ret_ == 1) printf("ID not found!\n");
				else if(ret_ == 0) printf("Deleted ID=%d\n", id);

                break;

            case 4:

                close(fd);
                return 0;
			default:
				printf("Invalid choice!\n");
				break;
        }
    }
}