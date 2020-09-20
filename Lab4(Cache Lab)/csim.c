/* Park Yegi, 20160253 */
#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

/* make struct for line, set, and cache */
typedef struct{
	int valid;
	int tag;
	int LRU_count;
}Line;
typedef struct{
	Line* lines;
}Set;
typedef struct{
	Set* sets;
}Cache;


void search(Cache *cache, int s, int S, int E, int b, long long address, int verbose, int *hit_count, int *miss_count, int *eviction_count);

int main(int argc, char **argv){
	int *hit_count = malloc(sizeof(*hit_count));
	int *miss_count = malloc(sizeof(*miss_count));
	int *eviction_count = malloc(sizeof(*eviction_count));
	int s, S, E, b;
	int c, i, j;
	int verbose = 0;
	FILE * fp;
	char operation;
	long long address;
	int size;

	/* get command line argument using getopt */
	while ((c = getopt(argc, argv, "hvs:E:b:t:")) != -1){
		switch(c)
		{
			case 'h':
				break;
			case 'v':
				verbose = 1;
				break;
			case 's':
				s = atoi(optarg);
				S = 1 << s;
				break;
			case 'E':
				E = atoi(optarg);
				break;
			case 'b':
				b = atoi(optarg);
				break;
			case 't':
			{
				fp = fopen(optarg, "r");
				break;
			}
		}
	}
	
	
	/* allocate space to cache and initialize */
	Cache *cache = malloc(sizeof(*cache));;
	cache->sets = (Set *)malloc(S*sizeof(Set));
	for (i=0; i<S; i++){
		cache->sets[i].lines = (Line *)malloc(E*sizeof(Line));
		for (j=0; j<E; j++){
			cache->sets[i].lines[j].valid = 0;
			cache->sets[i].lines[j].LRU_count = 0;
		}
	}

	
	/* run simulation line by line */
	while (fscanf(fp, " %c %llx,%d", &operation, &address, &size) > 0){
		if (operation != 'I' && verbose == 1)
			printf("%c %llx,%d ", operation, address, size);
		switch(operation){
			case 'I':
				break;
			case 'L':
				search(cache, s, S, E, b, address, verbose, hit_count, miss_count, eviction_count);
				break;
			case 'S':
				search(cache, s, S, E, b, address, verbose, hit_count, miss_count, eviction_count);
				break;
			case 'M':
				search(cache, s, S, E, b, address, verbose, hit_count, miss_count, eviction_count);
				search(cache, s, S, E, b, address, verbose, hit_count, miss_count, eviction_count);
				break;
			default:
				break;
		}
		if (verbose == 1)
			printf("\n");
	}

	fclose(fp);
	printSummary(*hit_count, *miss_count, *eviction_count);
	
	/* release malloc memory */
	for (i=0; i<S; i++)
		free(cache->sets[i].lines);
	free(cache->sets);
	free(cache);
	free(hit_count);
	free(miss_count);
	free(eviction_count);

	return 0;
}


void search(Cache *cache, int s, int S, int E, int b, long long address, int verbose, int *hit_count, int *miss_count, int *eviction_count){
	int i;
	int hit_or_miss; // 1 if hit, 0 if miss
	int tag_mask = (~0) << (s+b);
	int set_mask = ((~0)<<b) & (~tag_mask);
	int set_num = (address & set_mask) >> b;
	int tag_num = (address & tag_mask) >> (b+s);
	int line_num;
	int max_LRU_count_line; // line number with max LRU count
	int max_LRU_count;
	int evict_or_not = 1; //1 if eviction, 0 otherwise

	for (i=0; i<E; i++){
		if (cache->sets[set_num].lines[i].valid == 1){
			if (cache->sets[set_num].lines[i].tag == tag_num){
			// cache hit
				hit_or_miss = 1;
				line_num = i;		
				break;
			}
		}
		if (i == E-1)
			// cache miss (+ eviction)
			hit_or_miss = 0;
	}

	
	if (hit_or_miss){
		/* cache hit */
		for (i=0; i<E; i++)
			cache->sets[set_num].lines[i].LRU_count += 1;
		cache->sets[set_num].lines[line_num].LRU_count = 0;
		*hit_count += 1;
		if (verbose == 1)
			printf("hit ");
	}
	else{
		/* cache miss */
		*miss_count += 1;
		for (i=0; i<E; i++)
			cache->sets[set_num].lines[i].LRU_count += 1;
		if (verbose==1)
			printf("miss ");
		for (i=0; i<E; i++){
			if (cache->sets[set_num].lines[i].valid == 0){
				cache->sets[set_num].lines[i].valid = 1;
				cache->sets[set_num].lines[i].tag = tag_num;
				cache->sets[set_num].lines[i].LRU_count = 0;
				evict_or_not = 0;
				break;
			}
		}

		if (evict_or_not){
			/* eviction */
			max_LRU_count_line = 0;
			max_LRU_count = cache->sets[set_num].lines[0].LRU_count;
			if (E > 1){
				for (i=1; i<E; i++){
					if (cache->sets[set_num].lines[i].LRU_count > max_LRU_count){
						max_LRU_count_line = i;
						max_LRU_count = cache->sets[set_num].lines[i].LRU_count;
					}
				}
			}
			cache->sets[set_num].lines[max_LRU_count_line].tag = tag_num;
			cache->sets[set_num].lines[max_LRU_count_line].LRU_count = 0;
			*eviction_count += 1;
			if (verbose == 1)
				printf("eviction ");
		}
	}
}

