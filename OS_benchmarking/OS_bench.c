#include<stdio.h>
#include<string.h>
#include<stdbool.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

int LAYOUT_ID = 0;
int THREADS = 100;
int BLOCKSIZE = 0;
char *FILENAME;
char *BENCH_TYPE;


void disk_read (char *filename, int threads, int block_size);
void memory_read (char *filename, int threads, int block_size);
float avg_latency (float arrof_latency[], int threads);
static int get_block_size(int layout_id);

int system(const char *command);
//void *gen_random(int threads, char *filename);
//void read (int threads, uint32_t block_size, char *reading_on, char *reading_type, char *file_size, int size, char *filename, int times);
//int n_block(char *filename);

static int get_block_size(int layout_id){
    if (layout_id == 1)
        return 4096;
    else if (layout_id == 2)
        return 8192;
    else if (layout_id == 3)
        return 16384;
    else if (layout_id == 4)
        return 32768;
    else if (layout_id == 5)
        return 65536; // 64KB
    else if (layout_id == 6)
        return 131072; // 128KB
    else if (layout_id == 7)
        return 262144; // 256KB
    else if (layout_id == 8)
        return 524288; // 512KB
    else if (layout_id == 9)
        return 1048576; // 1MB
    else if (layout_id == 10)
        return 2097152; // 2MB
    else if (layout_id == 11)
        return 4194304; // 4MB
    else if (layout_id == 12)
        return 8388608; // 8MB
    else if (layout_id == 13)
        return 16777216; // 16MB
    else 
        printf("ERROR: Layout id (%d) is too big!\n", layout_id);
    	return 0;
}

int randomm (int min, int max){
    int r;
    r = ((rand() % (max + 1 - min)) + min);

    return r;

}
unsigned long rand_interval(unsigned long min, unsigned long max)
{
    unsigned long r;
    r = ((rand() % (max + 1 - min)) + min)*randomm(1,5);

    return r;
}

void disk_read (char *filename, int threads, int block_size){
	
	double total_latency=0, avg_total_latency;
	struct timeval  start, end;
	int i =0;
    char buf[block_size];
    memset (buf, 0, block_size);
    unsigned long offset = 0;
    ssize_t ret = 0;
    int count = block_size;
    int max_offset;

	int fd ;
    
    for (; i < threads; i++)
    {
        system("sync; echo 3 > /proc/sys/vm/drop_caches && swapoff -a && swapon -a");
        fd = open(filename, O_RDONLY);
        if(fd < 0)
        {
            perror("open failed");    
        }

        //10737418240 -> 10 GB
        //2147483648 -> 2 GB // max rand()
        offset = rand_interval(1, 2147483648);
        max_offset = 10737418240 - block_size;
        while (offset>max_offset)
        {
            offset = rand_interval(1, 2147483648);
        }
    	gettimeofday(&start, NULL);
        ret = pread64(fd, buf, count, offset);
        gettimeofday(&end, NULL);
        total_latency += ((end.tv_sec - start.tv_sec) * 1000000) + (end.tv_usec - start.tv_usec);
    //printf("%s\n", buf );
    }
    
    printf("\n####### Reading process on-disk#######");
    printf("\nReading %s...",filename);
    printf("\n==========================");
    printf("\nN_THREAD  = %d ",threads);
    printf("\nBLOCK SIZE  = %d KB",block_size);
    
    avg_total_latency = (total_latency)/(threads);
    printf("\navg latency = %f us or %f ms", avg_total_latency, avg_total_latency/1000);
	printf("\nThroughput = %f MBps",(block_size*threads)/(total_latency));
	printf("\n==========================\n");
}


void memory_read (char *filename, int threads, int block_size){
    
    double total_latency=0;
    float arr_of_read_latency [threads], average_latency;
    struct timeval  start, end;
    int i =0;
    char buf[block_size];
    memset (buf, 0, block_size);
    unsigned long offset = 0;
    ssize_t ret = 0;
    int count = block_size;


    int fd = open(filename, O_RDONLY);
        if(fd < 0)
        {
            perror("open failed");    
        }
    
    for (; i < threads; i++)
    {
        
        offset = 0;
        gettimeofday(&start, NULL);
        ret = pread64(fd, buf, count, offset);
        gettimeofday(&end, NULL);
        arr_of_read_latency[i] = ((end.tv_sec - start.tv_sec) * 1000000) + (end.tv_usec - start.tv_usec);
    //printf("%s\n", buf );
    }
    average_latency = avg_latency(arr_of_read_latency, threads);
    
    printf("\n####### Reading process in-memory#######");
    printf("\nReading %s...",filename);
    printf("\n==========================");
    printf("\nN_THREAD  = %d ",threads);
    printf("\nBLOCK SIZE  = %d KB",block_size);
    printf("\navg latency = %f us or %f ms", average_latency, average_latency/1000);
    printf("\nThroughput = %f MBps",(block_size)/(average_latency));
    printf("\n==========================\n");
}

float avg_latency (float arrof_latency[], int threads){
    int i;
    float sum =0.0, avg;
    for (i = 1; i < threads; i++)
    {
        sum+=arrof_latency[i];
    }
    avg = sum/(threads-1);

    return avg ;
}


int main(int argc, char *argv[])
{

    LAYOUT_ID = atoi(argv[1]);
    BLOCKSIZE = get_block_size(LAYOUT_ID);
    FILENAME = argv[2];
    THREADS = atoi(argv[3]);

    disk_read(FILENAME, THREADS, BLOCKSIZE);
    memory_read(FILENAME, THREADS, BLOCKSIZE);


}




