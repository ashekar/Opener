#include <stdbool.h>

#include <stdio.h>

#include <string.h>

#define MAX_MESSAGE_SIZE 115

#define MESSAGE_MULTIPLE_OF_FIVE 5

#define INTEGER_CHUNK 5

#define MESSAGE_ID_INDEX 6

#define HEADER_AND_CHECKSUM_SIZE 15

#define DATA_START 11

#define SECOND_BYTE 8

#define THIRD_BYTE 16

#define FOURTH_BYTE 24

#define START_HEADER_SIZE 10

#define HEADER_SEPARATOR_INDEX 9

#define SIZE_OF_START_FLAG 5

#define INT_MIN -2147483648

int buildInteger(int startIndex, char *buffer){
     int newVal = 0;
     newVal = newVal | buffer[++startIndex];
     newVal = newVal | (buffer[++startIndex] << SECOND_BYTE);
     newVal = newVal | (buffer[++startIndex] << THIRD_BYTE);
     newVal = newVal | (buffer[++startIndex] << FOURTH_BYTE);

     return newVal;

}

int fillArrayWithDataAndReturnChecksum(int *array, int numberOfIntegers, char *buffer){

    int startIndex = DATA_START;

    for(int i = 0; i < numberOfIntegers; i++){

        int newVal = buildInteger(startIndex, buffer);
       
        array[i] = newVal;

        startIndex += 5;
    }

    int checkSum = buildInteger(startIndex, buffer);

    return checkSum;


}

int checkSumValidation(int *array, int numberOfIntegers){
    if(numberOfIntegers <= 0){
        return 0;
    }
    int XORValue = array[0];
    for(int i = 1; i < numberOfIntegers; i++){
        XORValue ^= array[i];
    }

    return XORValue;
}

int* startBytesFindIndex(char *buffer, int total, int lastBytes){

    int checkFurther = lastBytes;
    int* result = new int[2];
    for(int i = 0; i < 5; i++){
        if(buffer[i] == 0xFF){
            checkFurther++;
        }else{
            break;
        }
    }

    if(checkFurther == 5){
        result[0] = 0;
        result[1] = -1 * lastBytes;
        return result;
    }

    for(int i = 0; i < total - 5; i++){
        if(buffer[i] == 0xFF && 
           buffer[i + 1] == 0xFF && 
           buffer[i + 2] == 0xFF && 
           buffer[i + 3] == 0xFF && 
           buffer[i + 4] == 0xFF){
            result[0] = 0;
            result[1] = i;
            return result;
        }
    }

    int seen = 0;

    for(int i = total - 1; i > total - 6; total--){
        if(buffer[i] == 0xFF){
            seen++;
        }else{
            break;
        }
    }

    result[0] = 1;
    result[1] = seen;

    return result;
    
}

bool checkSeparator(char *buffer){
    if(buffer[HEADER_SEPARATOR_INDEX] != 0xFE){
        return false;
    }

    return true;
}

int main(){

    char *buffer = new char[MAX_MESSAGE_SIZE];
    int index = 0;
    int FSeen = 0;

    while(true){
        
        int numberOfBytesRead = read(buffer + index, MAX_MESSAGE_SIZE);

        index += numberOfBytesRead;

        if(index >= START_HEADER_SIZE){
            int* newPointer = startBytesFindIndex(buffer, index, FSeen);
            if(newPointer[0] == 0){
                if(newPointer[1] >= 0){
                    char* holderMessage = new char[MAX_MESSAGE_SIZE];
                    memcpy(holderMessage, buffer, MAX_MESSAGE_SIZE);
                    memcpy(buffer, holderMessage + newPointer[1], MAX_MESSAGE_SIZE - newPointer[1]);
                    index = 0;
                    FSeen = 0;
                }else{
                    char* holderMessage = new char[MAX_MESSAGE_SIZE];
                    int end = -1 * newPointer[1];
                    for(int i = 0; i < end; i++){
                        holderMessage[i] = 0xFF;
                    }
                    memcpy(holderMessage + end, buffer, MAX_MESSAGE_SIZE - end);
                    memcpy(buffer, holderMessage, MAX_MESSAGE_SIZE);
                }
            }else{
                FSeen = newPointer[1];
                index = 0;
                buffer = new char[MAX_MESSAGE_SIZE];
                continue;
            }
            
        }else{
            continue;
        }


        //Check to make sure buffer is full

        unsigned int messageID = buffer[MESSAGE_ID_INDEX];

        int messageSize = get_message_size_from_message_id(messageID);

        if((messageSize + SIZE_OF_START_FLAG) > index){
            continue;
        }

        //Whole message is present
        //We assume the beginning of the next message will not be sent until later.
        //I.e. messages cannot be attached together -- only chunked

        int totalPackageSize = messageSize + SIZE_OF_START_FLAG;

        int dataSize = totalPackageSize - HEADER_AND_CHECKSUM_SIZE;

        int numberOfIntegers = dataSize/INTEGER_CHUNK;

        int* array = new int[numberOfIntegers];

        int checkSum = fillArrayWithDataAndReturnChecksum(array, numberOfIntegers, buffer);

        int XORValue = checkSumValidation(array, numberOfIntegers);

        if(checkSum == XORValue){
            printf("CheckSum matches\n");

            //Message is being processed as is, with the variable buffer, due to some minor ambiguity in writeup.
            //I wrote an email but never received a response.

            char* finalMessage = new char[totalPackageSize];
            memcpy(finalMessage, buffer, totalPackageSize);

            process_message(buffer, messageID);
            char *tempMessage = new char[MAX_MESSAGE_SIZE];
            memcpy(tempMessage, buffer + totalPackageSize, MAX_MESSAGE_SIZE - totalPackageSize);
            memcpy(buffer, tempMessage, MAX_MESSAGE_SIZE);
            index = 0;
            FSeen = 0;
        }

        
    }


    return 0;

}
