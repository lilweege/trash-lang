#include "io.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>

FileWriter fwCreate(const char* filename) {
    return (FileWriter) {
        .fn = filename,
        .cap = WRITER_MIN_SIZE,
        .size = 0,
        .data = (char*) malloc(WRITER_MIN_SIZE)
    };
}

void fwDestroy(FileWriter fw) {
    writeFileOrCrash(fw.fn, fw.size, fw.data);
    free(fw.data);
}

bool fwGrow(FileWriter* fw) {
    size_t newCap = fw->cap << 1; // assume this will never overflow
    void* newData = realloc(fw->data, newCap);
    if (newData == NULL) {
        return false;
    }
    fw->cap = newCap;
    fw->data = newData;
    return true;
}


void fwWriteChunkOrCrash(FileWriter* fw, char* fmt, ...) {
#define TMP_BUF_SZ 1024
    static char tmpBuf[TMP_BUF_SZ];

    while (fw->size + TMP_BUF_SZ >= fw->cap) {
        if (!fwGrow(fw)) {
            // return false;
            assert(0 && "Alloc failed when resizing file writer buffer");
        }
    }

    va_list args;
    va_start(args, fmt);
    int numBytes = vsnprintf(tmpBuf, TMP_BUF_SZ, fmt, args);
    va_end(args);
    assert(numBytes < TMP_BUF_SZ && "Chunk write to file to big for tmp buf (> 256 bytes)");

    memcpy(fw->data+fw->size, tmpBuf, numBytes);
    fw->size += numBytes;
    // return true;
#undef TMP_BUF_SZ
}

void readFileOrCrash(const char* filename, size_t* outSize, char** outBuff) {
    int res = readFile(filename, outSize, outBuff);
    if (res != 0) {
        switch (res) {
            break; case -2: fprintf(stderr, "ERROR: Malloc failed\n");
            break; case -1: fprintf(stderr, "ERROR: File '%s' was too large (%zu bytes)\n", filename, *outSize);
            break; case  1: fprintf(stderr, "ERROR: Couldn't read file '%s': %s\n", filename, strerror(errno));
            break; case  2: fprintf(stderr, "ERROR: Unexpected end of file '%s'\n", filename);
            break; case  3: fprintf(stderr, "ERROR: Couldn't read file '%s'\n", filename);
            break; default: fprintf(stderr, "ERROR: Unkown error reading file '%s': %s\n", filename, strerror(errno));
        }
        exit(1);
    }
}

void writeFileOrCrash(const char* filename, size_t size, char* buff) {
    int res = writeFile(filename, size, buff);
    if (res != 0) {
        fprintf(stderr, "ERROR: Couldn't write to file '%s': %s\n", filename, strerror(errno));
        exit(1);
    }
}


// -2 malloc failed
// -1 malloc too big
// 0 success
// 1 file error (errno)
// 2 feof
// 3 ferror
int readFile(const char* filename, size_t* outSize, char** outBuff) {
    assert(outSize != NULL);
    assert(outBuff != NULL);

    // open file
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
        return 1;
    }
    
    // get file size
    if (fseek(fp, 0, SEEK_END) != 0) {
        return 1;
    }
    long size = ftell(fp);
    if (size == -1L) {
        return 1;
    }
    if (fseek(fp, 0, SEEK_SET) != 0) {
        return 1;
    }

    *outSize = size;
    if (size + 1 >= FILE_MALLOC_CAP) {
        return -1;
    }
    char* buff = (char*) malloc(size + 1);
    if (buff == NULL) {
        return -2;
    }
    
    // read content
    size_t numBytes = fread(buff, 1, size, fp);
    if (numBytes != (size_t)size) {
        free(buff);
        if (feof(fp)) {
            return 2;
        }
        else if (ferror(fp)) {
            return 3;
        }
        return 4; // this is probably unreachable
    }
    fclose(fp);
    buff[size] = 0;

    // caller is responsible for freeing this
    *outBuff = buff;
    return 0;
}


int writeFile(const char* filename, size_t size, char* buff) {
    // open file
    FILE* fp = fopen(filename, "wb");
    if (fp == NULL) {
        return 1;
    }

    size_t numBytes = fwrite(buff, 1, size, fp);

    if (numBytes != size) {
        return 1;
    }

    fclose(fp);
    return 0;
}
