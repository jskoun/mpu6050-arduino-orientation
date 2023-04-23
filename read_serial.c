//A program to read a given serial port at a given baudrate
#include <Windows.h> //to handle the serial communications
//notice that this is not usable outside of windows systems (e.g POSIX-compliant operating systems such as Linux/macOS)
#include <stdio.h>
#include <stdlib.h>
#include <time.h> //for the UNIX epoch timestamp
#include <conio.h> //for _getch()

int main(int argc, char *argv[])
{
    int baudrate, port;
    int overwrite = 0;

    if (argc==3) //Hopefully correct baudrate + COM port
    {
        baudrate = atoi(argv[1]); //using atoi instead of (int)char, to avoid casting to a different size
        port = atoi(argv[2]);
    }
    else if (argc==1) //Not specified in args, prompt user for input
    {
        printf("Enter the baudrate: ");
        scanf("%d", &baudrate);
        printf("Enter the COM port number (e.g. 1 for COM1): ");
        scanf("%d", &port);
        printf("Overwrite displayed data? (0/1): ");
        scanf("%d", &overwrite);
    }
    else 
    {
        printf("[ERROR]: Invalid arguments.");
        return 1;
    }

    HANDLE hSerial;
    DCB dcbSerialParams = { 0 };
    COMMTIMEOUTS timeouts = { 0 };
    //char buffer[256];
    DWORD bytesRead; //keeping track of the bytes in serial
    char serial_port[20];
    char stop_char; //variable to check if P is pressed
    sprintf(serial_port, "\\\\.\\COM%d", port); //format char[] variable to provide the serial port
    FILE* fp = fopen("serial_output.txt", "w"); //output file
    FILETIME ft;
    ULARGE_INTEGER uli;
    
    

    hSerial = CreateFile(serial_port, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hSerial == INVALID_HANDLE_VALUE)
    {
        printf("Error opening serial port."); //Mostly debugging
        return 1;
    }

    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams))
    {
        printf("Error getting serial port state.");
        CloseHandle(hSerial);
        return 1;
    }

    dcbSerialParams.BaudRate = baudrate; //9600 should be the predefined const DCB_9600, but it's not implemented.
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams))
    {
        printf("Error setting serial port state.");
        CloseHandle(hSerial);
        return 1;
    }
    //constants that dictate the timeout values of serial comm shenanigans
    //in short, give up if the port is unresponsive
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hSerial, &timeouts)) //check set
    {
        printf("Error setting timeouts.");
        CloseHandle(hSerial);
        return 1;
    }
    while (1)
    {
        char buffer[256] = ""; //theoretically the buffer is 7 16bit values, 6 commas and \n -> 21bytes
        int pos = 0;
        char c = 0;
        while(c != '\n') { //this will read the serial port line by line, to add the necessary timestamp
        //& operator returns the memory address of a variable
            if (ReadFile(hSerial, &c, 1, &bytesRead, NULL))
            {
                if(bytesRead > 0) 
                {
                    buffer[pos++] = c;
                }
            }
            else
            {
                printf("Error reading from serial port.");
                CloseHandle(hSerial);
                return 1;
            }
        } //line acquired
        buffer[pos-1] = '\0'; //null terminator, replaces \n
        GetSystemTimeAsFileTime(&ft); //get time
        uli.LowPart = ft.dwLowDateTime; //assign to large integer
        uli.HighPart = ft.dwHighDateTime;
        long long milli_timestamp = uli.QuadPart / 10000ULL; //milliseconds timestamp

        //printf("Read %d bytes from serial port: %s", bytesRead, buffer);
        if (overwrite)
        {
            printf("\r[%llu]-> %s", milli_timestamp, buffer); //console
            fflush(stdout);
        }
        else {printf("[%llu]-> %s \n", milli_timestamp, buffer);}

        fprintf(fp, "%llu, %s \n", milli_timestamp, buffer); //output file, include \n
        fflush(fp); //flush the buffer

        if (_kbhit())  //on keyboard hit
        {
            stop_char = _getch(); 
            if (stop_char == 'p') //pressing p, for plot
            { 
                break;
            }
        }
    }

    CloseHandle(hSerial);
    printf("Data Collection Complete. Initializing Plot..."); //implement call to plotting program?
    fclose(fp);

    int result;
    // Call the other executable program
    result = system("plotdata.exe");
    // Check the result of the command
    if (result == 0) {
        printf("Plot created.\n");
    } else {
        printf("Program failed to execute.\n");
    }
    return 0;
}
