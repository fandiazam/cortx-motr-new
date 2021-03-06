/* -*- C -*- */
/*
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */

/*
 * Example Motr application to create object, write to object,
 * read from objet, and then delete the object.
 */

/*
 * Please change the dir according to you development environment.
 *
 * How to build:
 * gcc -I/work/cortx-motr -I/work/cortx-motr/extra-libs/galois/include \
 *     -DM0_EXTERN=extern -DM0_INTERNAL= -Wno-attributes               \
 *     -L/work/cortx-motr/motr/.libs -lmotr                            \
 *     example1.c -o example1
 *
 * Please change the configuration according to you development environment.
 *
 * How to run:
 * LD_LIBRARY_PATH=/work/cortx-motr/motr/.libs/                              \
 * ./example1 172.16.154.179@tcp:12345:34:1 172.16.154.179@tcp:12345:33:1000 \
 *         "<0x7000000000000001:0>" "<0x7200000000000001:64>" 12345670
 */

#include "motr/client.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <math.h>

typedef struct {
    int id;
    char *argv5;
    /* latency in ms */
    float latency;
    uint32_t random;
} Arg;

struct m0_config motr_conf;
struct m0_client *m0_instance = NULL;

/*
pthread_barrier_t barr_read;
pthread_barrier_t barr_write;
pthread_barrier_t barr_delete;

updated by update_n_block() */
int N_BLOCK = 1;

/* 0 == false ; 1 == true ; provided by argv[6], argv[7], argv[8] */
int progress_mod = 20;
const static int ENABLE_LATENCY_LOG = 0; 
int ENABLE_WRITE = 0;
int ENABLE_READ = 0; /* erase Page Cache: free -h; sync; echo 1 > /proc/sys/vm/drop_caches; free -h */
int ENABLE_DELETE = 0;

sem_t n_thread_semaphore;
/* Parallelishm: number of thd allowed in a sema space */
int N_THD_SEMA = 1;
/* 1 Thread == 1 Request == has N_BLOCK (each block has BLOCK_SIZE KB) */
//const static int N_THREAD = 1000;
//52641

/* provided by argv[9], argv[10] */
int LAYOUT_ID =  1,     BLOCK_SIZE =   4096; // 128KB
/*
 *  LAYOUT_ID               (Ideal size)
 *      1,     BLOCK_SIZE =     4096; // 4KB
 *      2,     BLOCK_SIZE =     8192; // 8KB
 *      3,     BLOCK_SIZE =    16384; // 16KB
 *      4,     BLOCK_SIZE =    32768; // 32KB
 *      5,     BLOCK_SIZE =    65536; // 64KB
 *      6,     BLOCK_SIZE =   131072; // 128KB
 *      7,     BLOCK_SIZE =   262144; // 256KB
 *      8,     BLOCK_SIZE =   524288; // 512KB
 *      9,     BLOCK_SIZE =  1048576; // 1MB
 *     10,     BLOCK_SIZE =  2097152; // 2MB
 *     11,     BLOCK_SIZE =  4194304; // 4MB
 *     12,     BLOCK_SIZE =  8388608; // 8MB
 *     13,     BLOCK_SIZE = 16777216; // 16MB
 *     14,     BLOCK_SIZE = 33554432; // 32MB (got error [be/engine.c:312:be_engine_got_tx_open]  tx=0x7f8be8027148 engine=0x7ffcd2d93830 t_prepared=(385285,115850973) t_payload_prepared=131072 bec_tx_size_max=(262144,100663296) bec_tx_payload_max=2097152)
 */

static int update_n_block(){
    /* payload per request = block_size */
    return 1;
}

/* return ideal block size */
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
}

static int object_create(struct m0_container *container, struct m0_uint128 obj_id)
{
	struct m0_obj     obj;
	struct m0_client *instance;
	struct m0_op     *ops[1] = {NULL};
	int               rc;

	M0_SET0(&obj);
	instance = container->co_realm.re_instance;
	m0_obj_init(&obj, &container->co_realm, &obj_id,
		    LAYOUT_ID);

	rc = m0_entity_create(NULL, &obj.ob_entity, &ops[0]);
	if (rc != 0) {
		printf("Failed to create object: %d\n", rc);
		return rc;
	}

	m0_op_launch(ops, 1);
	rc = m0_op_wait(ops[0],
			M0_BITS(M0_OS_FAILED, M0_OS_STABLE),
			M0_TIME_NEVER);
	if (rc == 0)
		rc = ops[0]->op_rc;

	m0_op_fini(ops[0]);
	m0_op_free(ops[0]);
	ops[0] = NULL;

	m0_entity_fini(&obj.ob_entity);

	// printf("Object (id=%lu) creation result: %d\n",
	//        (unsigned long)obj_id.u_lo, rc);
	return rc;
}

static int object_open(struct m0_obj *obj, struct m0_uint128 obj_id)
{
	struct m0_op *ops[1] = {NULL};
	int           rc;

	rc = m0_entity_open(&obj->ob_entity, &ops[0]);
	if (rc != 0) {
		printf("Failed to open object: %d\n", rc);
		return rc;
	}

	m0_op_launch(ops, 1);
	rc = m0_op_wait(ops[0],
			M0_BITS(M0_OS_FAILED, M0_OS_STABLE),
			M0_TIME_NEVER);
	if (rc == 0)
		rc = ops[0]->op_rc;

	m0_op_fini(ops[0]);
	m0_op_free(ops[0]);
	ops[0] = NULL;

	/* obj is valid if open succeeded */
	// printf("Object (id=%lu) open result: %d\n",
	//        (unsigned long)obj_id.u_lo, rc);
	return rc;
}

static int alloc_vecs(struct m0_indexvec *ext,
		      struct m0_bufvec   *data,
		      struct m0_bufvec   *attr,
		      uint32_t            block_count,
		      uint32_t            block_size)
{
	int      rc;

	rc = m0_indexvec_alloc(ext, block_count);
	if (rc != 0)
		return rc;

	/*
	 * this allocates <block_count> * <block_size>  buffers for data,
	 * and initialises the bufvec for us.
	 */

	rc = m0_bufvec_alloc(data, block_count, block_size);
	if (rc != 0) {
		m0_indexvec_free(ext);
		m0_bufvec_free(data);
		return rc;
	}
	rc = m0_bufvec_alloc(attr, block_count, 1);
	if (rc != 0) {
		m0_indexvec_free(ext);
		m0_bufvec_free(data);
		return rc;
	}
	return rc;
}

static void prepare_ext_vecs(struct m0_indexvec *ext,
			     struct m0_bufvec   *data,
			     struct m0_bufvec   *attr,
			     uint32_t            block_count,
			     uint32_t            block_size,
			     uint64_t           *last_index,
			     char                c1,
			     char                c2,
			     char                c3
                 )
{
	int      i;

    /* The data are composed from 3 different randomized char, this to minimize 
     * cache hit.
     */

	for (i = 0; i < block_count/3; ++i) {
		ext->iv_index[i]       = *last_index;
		ext->iv_vec.v_count[i] = block_size;
		*last_index           += block_size;

		/* Fill the buffer with all `c`. */
		memset(data->ov_buf[i], c1, data->ov_vec.v_count[i]);
		/* we don't want any attributes */
		attr->ov_vec.v_count[i] = 0;
	}

	for (; i < block_count/2; ++i) {
		ext->iv_index[i]       = *last_index;
		ext->iv_vec.v_count[i] = block_size;
		*last_index           += block_size;

		/* Fill the buffer with all `c`. */
		memset(data->ov_buf[i], c2, data->ov_vec.v_count[i]);
		/* we don't want any attributes */
		attr->ov_vec.v_count[i] = 0;
	}

	for (; i < block_count; ++i) {
		ext->iv_index[i]       = *last_index;
		ext->iv_vec.v_count[i] = block_size;
		*last_index           += block_size;

		/* Fill the buffer with all `c`. */
		memset(data->ov_buf[i], c3, data->ov_vec.v_count[i]);
		/* we don't want any attributes */
		attr->ov_vec.v_count[i] = 0;
	}
}

static void cleanup_vecs(struct m0_indexvec *ext,
			 struct m0_bufvec   *data,
			 struct m0_bufvec   *attr)
{
	/* Free bufvec's and indexvec's */
	m0_indexvec_free(ext);
	m0_bufvec_free(data);
	m0_bufvec_free(attr);
}

static int write_data_to_object(struct m0_obj      *obj,
				struct m0_indexvec *ext,
				struct m0_bufvec   *data,
				struct m0_bufvec   *attr)
{
	int          rc;
	struct m0_op *ops[1] = { NULL };

	/* Create the write request */
	m0_obj_op(obj, M0_OC_WRITE, ext, data, attr, 0, 0, &ops[0]);
	if (ops[0] == NULL) {
		printf("Failed to init a write op\n");
		return -EINVAL;
	}

	/* Launch the write request*/
	m0_op_launch(ops, 1);

	/* wait */
	rc = m0_op_wait(ops[0],
			M0_BITS(M0_OS_FAILED,
				M0_OS_STABLE),
			M0_TIME_NEVER);
	rc = rc ? : ops[0]->op_sm.sm_rc;

	/* fini and release the ops */
	m0_op_fini(ops[0]);
	m0_op_free(ops[0]);
	// printf("Object write result: %d\n", rc);
	return rc;
}

static int object_write(struct m0_container *container, Arg *arg, struct m0_uint128 obj_id)
{
	struct m0_obj      obj;
	struct m0_client  *instance;
    struct timeval st, et;
    int rc = 0, elapsed;

        struct m0_indexvec ext;
        struct m0_bufvec   data;
        struct m0_bufvec   attr;

	uint64_t           last_offset = 0;

    /* These values will be dynamically to minimize cache hit */
    char MY_CHAR_1 = 'W';
    char MY_CHAR_2 = 'W';
    char MY_CHAR_3 = 'W';

	M0_SET0(&obj);
	instance = container->co_realm.re_instance;
	m0_obj_init(&obj, &container->co_realm, &obj_id,
		    LAYOUT_ID);

	rc = object_open(&obj, obj_id);
	if (rc != 0) {
		printf("Failed to open object: rc=%d\n", rc);
		return rc;
	}

	/*
	 * alloc & prepare ext, data and attr. We will write 4k * 2.
	 */
	rc = alloc_vecs(&ext, &data, &attr, N_BLOCK, BLOCK_SIZE);
	if (rc != 0) {
		printf("Failed to alloc ext & data & attr: %d\n", rc);
		goto out;
	}

    srand((unsigned)time(NULL));
    MY_CHAR_1 = '/' + (rand() % 74);
    MY_CHAR_2 = '/' + (rand() % 74);
    MY_CHAR_3 = '/' + (rand() % 74);

    prepare_ext_vecs(&ext, &data, &attr, N_BLOCK, BLOCK_SIZE, &last_offset, 
        MY_CHAR_1, MY_CHAR_2, MY_CHAR_3);

    // pthread_barrier_wait(&barr_write);
    gettimeofday(&st, NULL);
    if (ENABLE_LATENCY_LOG > 0)
        printf("Thread_%d write .. obj_id %d\n", arg->id, obj_id.u_lo);
    
    /* Start to write data to object */
	rc = write_data_to_object(&obj, &ext, &data, &attr);
    
    gettimeofday(&et, NULL);
    elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);
    arg->latency = elapsed/1000.0f;

    if (ENABLE_LATENCY_LOG > 0)
        printf("Finish writing data in : %d us or %.3f s  (%d - %d)\n", elapsed,
            elapsed/1000000.0f, st.tv_sec, et.tv_sec);
    // print_throughput(elapsed);
    //printf("[barr_write] Finish writing data in obj_id %d\n", obj_id.u_lo);
    //printf("%d\n",arg->id);

	cleanup_vecs(&ext, &data, &attr);

out:
	/* Similar to close() */
	m0_entity_fini(&obj.ob_entity);

    if (arg->id % progress_mod == 0){
	    printf("+");
        fflush(stdout);
    }

	return rc;
}

static int read_data_from_object(struct m0_obj      *obj,
				 struct m0_indexvec *ext,
				 struct m0_bufvec   *data,
				 struct m0_bufvec   *attr)
{
	int          rc;
	struct m0_op *ops[1] = { NULL };

	/* Create the read request */
	m0_obj_op(obj, M0_OC_READ, ext, data, attr, 0, 0, &ops[0]);
	if (ops[0] == NULL) {
		printf("Failed to init a read op\n");
		return -EINVAL;
	}

	/* Launch the read request*/
	m0_op_launch(ops, 1);

	/* wait */
	rc = m0_op_wait(ops[0],
			M0_BITS(M0_OS_FAILED,
				M0_OS_STABLE),
			M0_TIME_NEVER);
	rc = rc ? : ops[0]->op_sm.sm_rc;

	/* fini and release the ops */
	m0_op_fini(ops[0]);
	m0_op_free(ops[0]);
	// printf("Object read result: %d\n", rc);
	return rc;
}

static void verify_show_data(struct m0_bufvec *data,
			     char c)
{
	int i, j;
	for (i = 0; i < data->ov_vec.v_nr; ++i) {
		// printf("  Block %6d:\n", i);
		// printf("%.*s", (int)data->ov_vec.v_count[i],
		// 	       (char *)data->ov_buf[i]);
		// printf("\n");
		for (j = 0; j < data->ov_vec.v_count[i]; j++)
			if (((char*) data->ov_buf[i])[j] != c) {
				printf("verification failed at: %d:%d\n"
				       "Expected %c result %c\n",
					i, j, c, ((char*)data->ov_buf[i])[j]);
			}
	}
    // printf("Verified the correctness of %6d blocks\n", data->ov_vec.v_nr);
}

static void print_throughput(int elapsed_time){
    float rc = 0;
    float size_kb = N_BLOCK * BLOCK_SIZE / 1000;
    // printf("data size %.2f KB (%d)\n", size_kb, N_BLOCK * BLOCK_SIZE);
    // rc = N_BLOCK * BLOCK_SIZE * 1000 / elapsed_time ;
    rc = size_kb * 1000000 / elapsed_time;
    printf("Throughput = %.2lf KB/sec\n", rc);
}

static int object_read(struct m0_container *container, 
    Arg *arg, struct m0_uint128 obj_id)
{
	struct m0_obj      obj;
    struct m0_client *instance;
    struct timeval st, et;
    int rc, elapsed;

    struct m0_indexvec ext;
    struct m0_bufvec data;
    struct m0_bufvec attr;

	uint64_t           last_offset = 0;

    // BUG from example1.c
    // rc = object_write(&motr_container, obj_id);
    
    // pthread_barrier_wait(&barr_read); 
    
    if (ENABLE_LATENCY_LOG > 0)
        printf("Thread_%d read .. obj_id %d\n", arg->id, obj_id.u_lo);

	M0_SET0(&obj);
	instance = container->co_realm.re_instance;
	m0_obj_init(&obj, &container->co_realm, &obj_id,
		    LAYOUT_ID);

	rc = object_open(&obj, obj_id);
	if (rc != 0) {
		printf("Failed to open object: rc=%d\n", rc);
		return rc;
	}

	/*
	 * alloc & prepare ext, data and attr. We will write 4k * 2.
	 */
    rc = alloc_vecs(&ext, &data, &attr, N_BLOCK, BLOCK_SIZE);
    if (rc != 0) {
		printf("Failed to alloc ext & data & attr: %d\n", rc);
		goto out;
	}
    prepare_ext_vecs(&ext, &data, &attr, N_BLOCK, BLOCK_SIZE, &last_offset, '\0','\0','\0');
    
    gettimeofday(&st, NULL);
    /* Start to read data to object */
	rc = read_data_from_object(&obj, &ext, &data, &attr);
    
    gettimeofday(&et, NULL);
    elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);
    arg->latency = elapsed/1000.0f;
    //printf("[barr_read] Finish reading data in obj_id %d\n", obj_id.u_lo);
    //printf("%d\n",obj_id.u_lo);

    if (ENABLE_LATENCY_LOG > 0)
        printf("Finish reading data in : %d us or %.3f s  (%d - %d)\n", elapsed,
                elapsed/1000000.0f, st.tv_sec, et.tv_sec);
    // print_throughput(elapsed);

    // if (rc == 0) {
    //     verify_show_data(&data, MY_CHAR_1);
    // }
	cleanup_vecs(&ext, &data, &attr);

out:
	/* Similar to close() */
	m0_entity_fini(&obj.ob_entity);
    if (arg->id % progress_mod == 0){
	    printf("+");
        fflush(stdout);
    }

	// printf("Object read: %d\n", rc);
	return rc;
}

static int object_delete(struct m0_container *container, struct m0_uint128 obj_id)
{
	struct m0_obj      obj;
	struct m0_client  *instance;
	struct m0_op      *ops[1] = { NULL };
	int                rc;

	M0_SET0(&obj);
	instance = container->co_realm.re_instance;
	m0_obj_init(&obj, &container->co_realm, &obj_id,
		    LAYOUT_ID);

	rc = object_open(&obj, obj_id);
	if (rc != 0) {
		printf("Failed to open object: rc=%d\n", rc);
		return rc;
	}

	m0_entity_delete(&obj.ob_entity, &ops[0]);
	//printf("[barr_delete] Finish delete data in obj_id %d\n", obj_id.u_lo);
	m0_op_launch(ops, 1);
	rc = m0_op_wait(ops[0],
			M0_BITS(M0_OS_FAILED, M0_OS_STABLE),
			M0_TIME_NEVER);

	/* fini and release */
	m0_op_fini(ops[0]);
	m0_op_free(ops[0]);

	/* Similar to close() */
	m0_entity_fini(&obj.ob_entity);

	// printf("Object deletion: %d\n", rc);
	return rc;
}

uint32_t rand_interval(uint32_t min, uint32_t max)
{
    uint32_t r;
    r = ((rand() % (max + 1 - min)) + min);

    return r;
}

static int start_each_thread(struct m0_uint128 obj_id, Arg *arg)
{
    struct m0_container motr_container;
    struct timeval st, et;
    int rc = 0, elapsed = 0;
    sem_wait(&n_thread_semaphore);
    uint32_t rand_val;

    m0_container_init(&motr_container, NULL, &M0_UBER_REALM, m0_instance);
    rc = motr_container.co_realm.re_entity.en_sm.sm_rc;
    if (rc != 0)
    {
        printf("error in m0_container_init: %d\n", rc);
        goto out;
    }

    if (ENABLE_WRITE > 0) {
        rc = object_create(&motr_container, obj_id);
    }
    if (rc == 0)
    {   
        if (ENABLE_WRITE > 0) {
            rc = object_write(&motr_container, arg, obj_id);

        }
        if (ENABLE_READ > 0 && rc == 0)
        {
        	//rand_val = rand_interval(1, 2621440);
    		//rand_val = rand_interval(1, 128000);
        	//obj_id.u_lo = atoll(arg->argv5)+arg->id;
        	//system("sync; echo 1 > /proc/sys/vm/drop_caches");
            rc = object_read(&motr_container, arg, obj_id);
        }
        if ( ENABLE_DELETE > 0) {
            // pthread_barrier_wait(&barr_delete);
            if (ENABLE_LATENCY_LOG > 0)
                printf("Thread_%d delete .. obj_id %d\n", arg->id, obj_id.u_lo);
            object_delete(&motr_container, obj_id);
        }
    }
    sem_post(&n_thread_semaphore);

out:
    return elapsed;
}

static void* init_client_thread(void *vargp) {
    struct m0_uint128 obj_id = {0, 0};
    Arg *arg = (Arg *)vargp;

    /* Update 5th argument to create different object ids among the threads 
    if (obj_id.u_lo < M0_ID_APP.u_lo) {
        printf("obj_id invalid. Please refer to M0_ID_APP "
               "in motr/client.c\n");
        exit(-EINVAL);
    }
	*/
    // printf("This is thread.. %d\n", arg->id);
    if (ENABLE_WRITE > 0 || ENABLE_DELETE > 0)
    {
    	obj_id.u_lo = atoll(arg->argv5)+arg->id;
    	start_each_thread(obj_id, arg);
    }
    
    if (ENABLE_READ > 0)
    {
    	//obj_id.u_lo = atoll(arg->argv5)+arg->id;

    	obj_id.u_lo = atoll(arg->argv5)+arg->random;
    	start_each_thread(obj_id, arg);
    }

    pthread_exit(NULL);
}

static void print_setup(unsigned int N_THREAD){
    printf("====================\n");
    printf("Write/Read/Delete\n  %d  /  %d /   %d\n", ENABLE_WRITE, ENABLE_READ, ENABLE_DELETE);
    printf("N_THREAD   : %d\n", N_THREAD);
    printf("N_THD_SEMA : %d\n", N_THD_SEMA);
    printf("N_BLOCK    : %d\n", N_BLOCK);
    printf("BLOCK_SIZE : %d KB\n", BLOCK_SIZE/1024);
    printf("TOTAL_SIZE : %d KB (%.2f MB)\n", N_BLOCK*BLOCK_SIZE/1024*N_THREAD,  N_BLOCK*BLOCK_SIZE/1024/1024.0f*N_THREAD);
    printf("====================\n");
}
/*
void create_thread(uint32_t counter_creating_thread, uint32_t *t[], Arg * array_arg, char *argv[]){
	int i;

	for (i = 0; i < 100; i++){
			counter_creating_thread += 1;
	    	array_arg[counter_creating_thread].id = counter_creating_thread;
	        array_arg[counter_creating_thread].argv5 = argv[5];
	        //array_arg[counter_creating_thread].latency = 0;
		    array_arg[counter_creating_thread].random = rand_interval(1, 23000);
		    pthread_create(&t[counter_creating_thread], NULL, init_client_thread, &array_arg[counter_creating_thread]);
	    	}
}
*/

int main(int argc, char *argv[])
{
	//system("sync; echo 1 > /proc/sys/vm/drop_caches");
    struct m0_idx_dix_config motr_dix_conf;
    struct timeval st, et;
    int rc = 0;
    double sum_latency = 0, elapsed = 0, total_payload = 0;
	uint32_t N_THREAD;
    int batch_thread,times, i, count=0, roundd=0, write_batch_thread;
    int now_thread=0, first_batch_execution, sec_batch_execution;
    int loop, j, residue, count_thread_done=0;


    /* Initiate the IO mode */
    ENABLE_WRITE = atoi(argv[6]);
    ENABLE_READ = atoi(argv[7]);
    ENABLE_DELETE = atoi(argv[8]);
    // Specify the Layout and block size
    LAYOUT_ID = atoi(argv[9]); 
    N_THD_SEMA = atoi(argv[10]);
    N_THREAD = atoi(argv[11]);
    times = atoi(argv[12]);

    Arg array_arg[N_THREAD];
    sem_init(&n_thread_semaphore, 0, N_THD_SEMA);

    /* To print out progress every 5 % */
    if (N_THREAD > 20)
        progress_mod = N_THREAD/20;

    /* Make it dynamic for benchmarking */
    BLOCK_SIZE = get_block_size(LAYOUT_ID); 
    N_BLOCK = update_n_block();
    print_setup(N_THREAD);

    if (argc < (6 + 3 )) {
		printf("Need more arguments: %s HA_ADDR LOCAL_ADDR Profile_fid Process_fid obj_id\n",
		       argv[0]);
		exit(-1);
	}
    
    motr_dix_conf.kc_create_meta = false;
    motr_conf.mc_is_oostore = true;
    motr_conf.mc_is_read_verify = false;
    motr_conf.mc_ha_addr = argv[1];
    motr_conf.mc_local_addr = argv[2];
    motr_conf.mc_profile = argv[3];
    motr_conf.mc_process_fid = argv[4];
    motr_conf.mc_tm_recv_queue_min_len = M0_NET_TM_RECV_QUEUE_DEF_LEN;
    motr_conf.mc_max_rpc_msg_size = M0_RPC_DEF_MAX_RPC_MSG_SIZE;
    motr_conf.mc_idx_service_id = M0_IDX_DIX;
    motr_conf.mc_idx_service_conf = (void *)&motr_dix_conf;

    rc = m0_client_init(&m0_instance, &motr_conf, true);
    if (rc != 0)
    {
        printf("error in m0_client_init: %d\n", rc);
        exit(rc);
    }

    pthread_t t[N_THREAD];
    //pthread_barrier_init(&barr_read, NULL, N_THREAD);
    //pthread_barrier_init(&barr_write, NULL, N_THREAD);
    //pthread_barrier_init(&barr_delete, NULL, N_THREAD);
    
    gettimeofday(&st, NULL);
    if (ENABLE_WRITE>0 || ENABLE_DELETE>0)
    {
    	write_batch_thread = 10;
		while (count_thread_done<N_THREAD){		
	    	//create_thread(now_thread, &arr, array_arg, argv);
	    	for (i = 0; i < write_batch_thread; i++){
			    array_arg[count_thread_done].id = count_thread_done;
			    array_arg[count_thread_done].argv5 = argv[5];
			    array_arg[count_thread_done].latency = 0;
				//array_arg[now_thread].random = rand_interval(1, 9000);
				pthread_create(&t[count_thread_done], NULL, init_client_thread, &array_arg[count_thread_done]);
	        	pthread_join(t[count_thread_done], NULL);
	    		count_thread_done +=1;	
		    }
		    roundd+=1;	    
	  		printf("round = %i\n", roundd );
		}
    }
    else if (ENABLE_READ>0)
    {
    	for (i = 0; i < N_THREAD; i++)
	    {
	        array_arg[i].random = rand_interval(1, 25000);
	    }

	    if (N_THD_SEMA%2==0)
	    {
	    	batch_thread=floor(times*N_THD_SEMA);
	    	first_batch_execution = floor(batch_thread/2);
	    	sec_batch_execution = batch_thread - first_batch_execution;
	    }
	    else
	    {
	    	batch_thread=floor(times*N_THD_SEMA);
	    	first_batch_execution = floor(batch_thread/2);
	    	sec_batch_execution = batch_thread - first_batch_execution;
	    }

	    printf("first_batch_execution = %i \n", first_batch_execution );
	    printf("sec_batch_execution = %i\n", sec_batch_execution );
	    
	    
	    for (i = 0; i < batch_thread; i++){
	        array_arg[i].id = i+1;
	        array_arg[i].argv5 = argv[5];
	        array_arg[i].latency = 0;
	        pthread_create(&t[i], NULL, init_client_thread, &array_arg[i]);
	        
	    }
	    loop = floor(N_THREAD/batch_thread);
	    for (j = 1; j <= loop; j++)
	    {
		    for (i = 0; i < first_batch_execution; i++){
		        pthread_join(t[count_thread_done], NULL);
		    	count_thread_done +=1;
		    }
		    if (((count_thread_done+sec_batch_execution)<=N_THREAD))
		    {	
		    	now_thread=count_thread_done+sec_batch_execution;
		    	if ((now_thread+batch_thread)>N_THREAD)
		    	{
		    		residue = N_THREAD-now_thread;
		    		for (i = 0; i < residue; i++){
					    array_arg[now_thread].id = now_thread;
					    array_arg[now_thread].argv5 = argv[5];
					    array_arg[now_thread].latency = 0;
						pthread_create(&t[now_thread], NULL, init_client_thread, &array_arg[now_thread]);
						now_thread += 1;
				    }
		    	}
		    	else
		    	{
			    	for (i = 0; i < batch_thread; i++){
					    array_arg[now_thread].id = now_thread;
					    array_arg[now_thread].argv5 = argv[5];
					    array_arg[now_thread].latency = 0;
						pthread_create(&t[now_thread], NULL, init_client_thread, &array_arg[now_thread]);
						now_thread += 1;
					}	
				} 
		    }
		    for (i = 0; i < sec_batch_execution; i++){
		        pthread_join(t[count_thread_done], NULL);
		    	count_thread_done +=1;	
		    }

	    	if (j==loop)
	    	{
			    for (i = 0; i < residue; i++){
			        pthread_join(t[count_thread_done], NULL);
			    	count_thread_done +=1;
			    }     		
	    	}
		}
	}

    gettimeofday(&et, NULL);

    //pthread_barrier_destroy(&barr_read);
    //pthread_barrier_destroy(&barr_write);
    //pthread_barrier_destroy(&barr_delete);
    sem_destroy(&n_thread_semaphore);
    m0_client_fini(m0_instance, true);


    char *filename = "sum_latency_disk.txt";
    FILE *fp = fopen(filename, "w");
    // open the file for writing
    if (fp == NULL)
    {
        printf("Error opening the file %s", filename);
  		return -1;
    }

      
    /* Get the average latency */
    for (i = 0; i < N_THREAD; i++){
        sum_latency += array_arg[i].latency;
        fprintf(fp, "%.3f\n", array_arg[i].latency);
        }

    // close the file
    fclose(fp);
    //sum_latency=sum_latency/1000.0f;
    total_payload = N_BLOCK*BLOCK_SIZE/1048576.0f*N_THREAD;

    printf("\nAvg latency = %.5f/%d = %.5f ms; latency = %.5f s \n",
        sum_latency,N_THREAD, sum_latency/N_THREAD, sum_latency/1000.0f);

	//printf("Throughput = %.2f/%.2f = %.2f MBps\n",total_payload,(sum_latency), total_payload/(sum_latency));
	printf("=====================\n");
    /* Calculate throughput in MBps */
    elapsed = (((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec)-30)/1000000.0f;
    //elapsed = (((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec))/1000000.0f ;
    printf("Throughput = %.2f/%.2f = %.2f MBps\n",total_payload,elapsed, total_payload/elapsed);

    // print_setup();
    printf("app completed: %d\n", rc);
}

/*
 And there are multiple "m0_instance" in this program (in multiple threads).
This is not allowed. Only a single instance is allowed in a process space address.
Please share this instance in all threads.
You can refer to source code: motr/st/utils/copy_mt.c
You will find how to do multi-threaded Motr client app.

Please do this in another file: e.g. example_mt.c
And write some guide for how to program a multi-threaded Motr application.
And then create a new PR.
*/

/*
 *  Local variables:
 *  c-indentation-style: "K&R"
 *  c-basic-offset: 8
 *  tab-width: 8
 *  fill-column: 80
 *  scroll-step: 1
 *  End:
 */
/*
 * vim: tabstop=8 shiftwidth=8 noexpandtab textwidth=80 nowrap
 */
