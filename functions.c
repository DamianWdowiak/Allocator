#include "functions.h"

heap_t *heap = NULL;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int heap_setup(void){
    pthread_mutex_lock(&mutex);
    if(heap != NULL)
        return 0;
    
    intptr_t start = (intptr_t)custom_sbrk(0);
    if(custom_sbrk(PAGE_SIZE) == (void *) -1) 
        return -1;
    heap = (heap_t*)start;
    heap->brk = (intptr_t)custom_sbrk(0);
    heap->start_brk = start;
    heap->first = (block_t*)(heap->start_brk + sizeof(heap_t));
    block_t *block_second= (block_t*)(heap->start_brk + sizeof(heap_t) + sizeof(block_t));
    heap->last = (block_t*)(heap->brk - sizeof(block_t));
    heap->first->prev = NULL;
    heap->first->next = (block_t*)block_second;
    heap->first->size = 0;
    heap->first->filename = NULL;
    heap->first->user_size = 0;
    heap->first->line = 0;
    block_second->prev = (block_t*)heap->first;
    block_second->next = (block_t*)heap->last;
    block_second->size = -(heap->brk - heap->start_brk - 3 * sizeof(block_t) - 2 * sizeof(struct block_fence_t) - sizeof(heap_t)); 
    block_second->user_size = 0; 
    block_second->filename = NULL;
    block_second->line = 0;
    heap->last->prev = (block_t*)block_second;
    heap->last->next = NULL;
    heap->last->size = 0;
    heap->last->filename = NULL;
    heap->last->user_size = 0;
    heap->last->line = 0;
    
    for(int i = 0; i<8; ++i){
        block_second->left_fence.pattern[i] = i+1;
        block_second->right_fence.pattern[i] = 8 - i;
    }
    uint8_t *base = (uint8_t*)block_second + sizeof(block_t);
    *(struct block_fence_t*)base = block_second->left_fence;
    *(struct block_fence_t*)(base + sizeof(struct block_fence_t) - block_second->size) = block_second->right_fence;
    update_checksum(heap->first);
    update_checksum(heap->last);
    update_checksum(block_second);
    update_checksum_heap(heap);
    pthread_mutex_unlock(&mutex);
    return 0;
}

void* heap_malloc(size_t count){
    if(count<=0)
        return NULL;
    if(heap == NULL){
        if(heap_setup() == -1){
            return NULL;
        }
    }
    if(heap_validate() != 0)
        exit(1);

    bool found = false;
    pthread_mutex_lock(&mutex);
    block_t *block = heap->first->next;
    ptrdiff_t temp_count=count;
    temp_count*=-1;
    while(block != heap->last){ 
        if(block->size <= temp_count){
            found = true;
            break;
        }
        block = block->next;
    }
    if(found){
        block->size *= -1;
        size_t size = (count + (sizeof(void*)-1)) & ~ (sizeof(void*)-1);
        if(size + sizeof(block_t) + 2*sizeof(struct block_fence_t) + sizeof(void*) <= block->size){
            uint8_t *base = (uint8_t*)block + sizeof(block_t);
            *(struct block_fence_t*)(base + sizeof(struct block_fence_t) + count) = block->right_fence;
            block_t *new_block = (block_t*)((intptr_t)block + sizeof(block_t) + 2*sizeof(struct block_fence_t) + size);
            new_block->prev = block;
            new_block->next = block->next;
            block->next->prev = new_block;
            new_block->filename = NULL;
            new_block->size = (block->size - size - sizeof(block_t) - 2*sizeof(struct block_fence_t))*(-1);
            block->next = new_block;
            block->size = size;
            for(int i = 0; i<8; ++i){
                new_block->left_fence.pattern[i] = i+1;
                new_block->right_fence.pattern[i] = 8 - i;
            }
            base = (uint8_t*)new_block + sizeof(block_t);
            *(struct block_fence_t*)base = new_block->left_fence;
            *(struct block_fence_t*)(base + sizeof(struct block_fence_t) - new_block->size) = new_block->right_fence;
            update_checksum(new_block);
            update_checksum(new_block->next);
        }
        block->filename = NULL;
        block->user_size=count;
        for(int i = 0; i<8; ++i){
            block->left_fence.pattern[i] = i+1;
            block->right_fence.pattern[i] = 8 - i;
        }
        uint8_t *base = (uint8_t*)block + sizeof(block_t);
        *(struct block_fence_t*)base = block->left_fence;
        *(struct block_fence_t*)(base + sizeof(struct block_fence_t) + block->user_size) = block->right_fence;
        update_checksum(block);
        update_checksum_heap(heap);
        pthread_mutex_unlock(&mutex);
        return (void*)block + sizeof(block_t) + sizeof(struct block_fence_t);
    }
    size_t size = 0;
    if(heap->last->prev->size < 0){
        block = heap->last->prev;
        size = (count + block->size + (PAGE_SIZE-1)) & ~ (PAGE_SIZE-1);
        if(custom_sbrk(size) == (void *) -1){
            pthread_mutex_unlock(&mutex);
            return NULL;
        }
        heap->brk = (intptr_t)custom_sbrk(0);
        block->size -= size;
        for(int i = 0; i<8; ++i){
            block->left_fence.pattern[i] = i+1;
            block->right_fence.pattern[i] = 8 - i;
        }
        uint8_t *base = (uint8_t*)block + sizeof(block_t);
        *(struct block_fence_t*)base = block->left_fence;
        *(struct block_fence_t*)(base + sizeof(struct block_fence_t) + count) = block->right_fence;
        heap->last = (block_t*)(heap->brk - sizeof(block_t));
        block->next = heap->last;
        heap->last->prev = block;
        heap->last->next = NULL;
        heap->last->size = 0;
        block->filename = NULL;
//        block->user_size=count;
        block->user_size=0;
        update_checksum(block);
        update_checksum(heap->last);
    }
    else{
        size = (count + sizeof(block_t) + 2 * sizeof(struct block_fence_t) + (PAGE_SIZE-1)) & ~ (PAGE_SIZE-1); //size zaokrąglony do wielokrotności wilekości PAGE_SIZE
        if(custom_sbrk(size) == (void *) -1){
            pthread_mutex_unlock(&mutex);
            return NULL;
        }
        heap->brk = (intptr_t)custom_sbrk(0);
        
        block = heap->last;
        block->prev = heap->last->prev;
        block->size = size - sizeof(block_t) - 2 * sizeof(struct block_fence_t);
        for(int i = 0; i<8; ++i){
            block->left_fence.pattern[i] = i+1;
            block->right_fence.pattern[i] = 8 - i;
        }
        uint8_t *base = (uint8_t*)block + sizeof(block_t);
        *(struct block_fence_t*)base = block->left_fence;
        *(struct block_fence_t*)(base + sizeof(struct block_fence_t) + count) = block->right_fence;
        
        heap->last = (block_t*)(heap->brk - sizeof(block_t));
        block->next = heap->last;
        heap->last->size = 0;
        heap->last->next = NULL;
        heap->last->prev = block;
        block->filename = NULL;
        block->user_size=0;
        block->size *=-1;
        update_checksum(block);
        update_checksum(heap->last);
    }
    update_checksum_heap(heap);
    pthread_mutex_unlock(&mutex);
//    return (void*)block+sizeof(struct block_fence_t) +sizeof(block_t);
    return heap_malloc(count);
}
void* heap_calloc(size_t number, size_t size){
    if(number<=0 || size<=0)
        return NULL;
    size_t n_size = number * size;
    if(size != n_size / number) //overflow
        return NULL;
    void * ptr = heap_malloc(n_size);
    if(!ptr)
        return NULL;
    memset(ptr, 0, n_size);
    return ptr;
}

void heap_free(void* memblock){
    if(get_pointer_type(memblock) == pointer_valid){
        pthread_mutex_lock(&mutex);
        block_t *block = (block_t*)(memblock-sizeof(struct block_fence_t) - sizeof(block_t));
        //coalese
        if(block->size > 0){
            if(block->prev->size<0){
                block->filename = NULL;
                block = block->prev;
                block->size -= (block->next->size +sizeof(block_t) + 2*sizeof(struct block_fence_t));
                block->next->next->prev = block;
                block->next = block->next->next;
                block->size *= -1; //size na + 
            }
            if(block->next->size <0){
                block->next->filename = NULL;
                block->size += (-block->next->size +sizeof(block_t) + 2*sizeof(struct block_fence_t)); //size na +
                block->next->next->prev = block;
                block->next = block->next->next;
            }
            block->size *= -1;
            block->user_size = 0;
            block->line = 0;
            block->filename = NULL;
            update_checksum(block);
            update_checksum(block->next);
            update_checksum_heap(heap);
        }
        pthread_mutex_unlock(&mutex);
        if(!heap_get_used_blocks_count()){
            if(custom_sbrk(-heap_get_used_space()) == (void *) -1){
                return;
            }
            pthread_mutex_lock(&mutex);
            heap = NULL;
            pthread_mutex_unlock(&mutex);
            heap_setup();
        }
    }
}

void* heap_realloc(void* memblock, size_t size){
    if(heap_validate() != 0)
        exit(1);
    if(memblock && !size){
        heap_free(memblock);
        return NULL;
    }
    if(!memblock || size<=0)
        return heap_malloc(size);
    block_t *block = memblock -sizeof(struct block_fence_t) - sizeof(block_t);
    if(block->size >= size){
        block->user_size = size;
        uint8_t *base = (uint8_t*)block + sizeof(block_t);
        *(struct block_fence_t*)(base + sizeof(struct block_fence_t) + size) = block->right_fence;
        update_checksum(block);
        return memblock;
    }
    void *ptr = heap_malloc(size);
    if(ptr){
        memcpy(ptr,memblock,block->user_size);
        heap_free(memblock);
    }
    return ptr;
}

void* heap_malloc_debug(size_t count, int fileline, const char* filename){
    if(fileline < 0 || !filename)
        return NULL;
    void * ptr = heap_malloc(count);
    if(!ptr)
        return NULL;
    pthread_mutex_lock(&mutex);
    block_t* block = ptr-sizeof(block_t)-sizeof(struct block_fence_t);
    block->filename = filename;
    block->line = fileline;
    update_checksum(block);
    pthread_mutex_unlock(&mutex);
    return ptr;
}

void* heap_calloc_debug(size_t number, size_t size, int fileline, const char* filename){
    if(fileline < 0 || !filename)
        return NULL;
    void * ptr = heap_calloc(number,size);
    if(!ptr)
        return NULL;
    pthread_mutex_lock(&mutex);
    block_t* block = ptr-sizeof(block_t)-sizeof(struct block_fence_t);
    block->filename = filename;
    block->line = fileline;
    update_checksum(block);
    pthread_mutex_unlock(&mutex);
    return ptr;
}

void* heap_realloc_debug(void* memblock, size_t size, int fileline, const char* filename){
    if(fileline < 0 || !filename)
        return NULL;
    void * ptr = heap_realloc(memblock,size);
    if(!ptr)
        return NULL;
    pthread_mutex_lock(&mutex);
    block_t* block = ptr-sizeof(block_t)-sizeof(struct block_fence_t);
    block->filename = filename;
    block->line = fileline;
    update_checksum(block);
    pthread_mutex_unlock(&mutex);
    return ptr;
}

void* heap_malloc_aligned(size_t count){
    if(count<=0)
        return NULL;
    if(heap == NULL){
        if(heap_setup() == -1){
            return NULL;
        }
    }
    if(heap_validate() != 0)
        exit(1);
    bool found = false;
    pthread_mutex_lock(&mutex);
    block_t *block = heap->first->next;
    ptrdiff_t temp_count=count;
    temp_count*=-1;
    size_t new_block_size = 0;
    size_t size = (count + (sizeof(void*)-1)) & ~ (sizeof(void*)-1);
    while(block != heap->last){ 
        if(block->size <= temp_count){
            if((((intptr_t)block+sizeof(block_t) + sizeof(struct block_fence_t)) & (intptr_t)(PAGE_SIZE-1)) == 0){
                found = true;
                break;
            }
            for(new_block_size = 0; new_block_size<-(block->size + sizeof(block_t) + 2*sizeof(struct block_fence_t)+size) ;new_block_size+=sizeof(void*)){
                if((((intptr_t)block+new_block_size+3*sizeof(struct block_fence_t) +2*sizeof(block_t)) & (intptr_t)(PAGE_SIZE-1)) == 0){ 
                    found = true;
                    break;
                }
            }
        }
        if(found)
            break;
        new_block_size=0;
        block = block->next;
    }
    if(found){
        if(!new_block_size){
            block->size *= -1;
            if(size + sizeof(block_t) + 2*sizeof(struct block_fence_t) + sizeof(void*) <= block->size){
                uint8_t *base = (uint8_t*)block + sizeof(block_t);
                *(struct block_fence_t*)(base + sizeof(struct block_fence_t) + count) = block->right_fence;
                block_t *new_block = (block_t*)((intptr_t)block + sizeof(block_t) + 2*sizeof(struct block_fence_t) + size);
                new_block->prev = block;
                new_block->next = block->next;
                block->next->prev = new_block;
                new_block->size = (block->size - size - sizeof(block_t) - 2*sizeof(struct block_fence_t))*(-1);
                block->next = new_block;
                block->size = size;
                for(int i = 0; i<8; ++i){
                    new_block->left_fence.pattern[i] = i+1;
                    new_block->right_fence.pattern[i] = 8 - i;
                }
                base = (uint8_t*)new_block + sizeof(block_t);
                *(struct block_fence_t*)base = new_block->left_fence;
                *(struct block_fence_t*)(base + sizeof(struct block_fence_t) - new_block->size) = new_block->right_fence;
                new_block->filename = NULL;
                new_block->line = 0;
                update_checksum(new_block->next);
                update_checksum(new_block);
            }
        }
        else{
            size_t size = (count + (sizeof(void*)-1)) & ~ (sizeof(void*)-1); //potrzebny size (aligned_block)
            block_t *aligned_block = (block_t*)((intptr_t)block + new_block_size + 2*sizeof(struct block_fence_t)+sizeof(block_t));
            for(int i = 0; i<8; ++i){
                aligned_block->left_fence.pattern[i] = i+1;
                aligned_block->right_fence.pattern[i] = 8 - i;
            }
            aligned_block->size = size;
            aligned_block->prev = block;
            aligned_block->filename = NULL;
            aligned_block->user_size=count;
            aligned_block->line=0;
            uint8_t *base = (uint8_t*)aligned_block + sizeof(block_t);
            *(struct block_fence_t*)base = aligned_block->left_fence;
            *(struct block_fence_t*)(base + sizeof(struct block_fence_t) + aligned_block->user_size) = aligned_block->right_fence;
            if(block->size + new_block_size + sizeof(block_t) + size + 2*sizeof(struct block_fence_t) <= -(sizeof(block_t)+ 2*sizeof(struct block_fence_t)+sizeof(void*))){
                block_t *after_block = (block_t*)((intptr_t)aligned_block + sizeof(block_t) + size + 2*sizeof(struct block_fence_t));
                after_block->size = block->size + new_block_size + 2*sizeof(block_t) + size + 4*sizeof(struct block_fence_t);
                aligned_block->next = after_block;
                after_block->next = block->next;
                block->next->prev = after_block;
                after_block->prev = aligned_block;
                block->next = aligned_block;
                block->size = new_block_size;
                block->size *= -1;
                for(int i = 0; i<8; ++i){
                    after_block->left_fence.pattern[i] = i+1;
                    after_block->right_fence.pattern[i] = 8 - i;
                }
                after_block->filename = NULL;
                after_block->line = 0;
                after_block->user_size = 0;
                update_checksum(after_block);
                update_checksum(after_block->next);
            }//brak miejsca na kolejny blok
            else{
                aligned_block->next=block->next;
                block->next->prev = aligned_block;
                block->next = aligned_block;
                update_checksum(block->next);
            }
            aligned_block->filename=NULL;
            update_checksum(block);
            update_checksum(aligned_block);
            update_checksum_heap(heap);
            pthread_mutex_unlock(&mutex);
            return (void*)aligned_block + sizeof(block_t) + sizeof(struct block_fence_t);
        }
        block->filename = NULL;
        block->user_size = count;
        uint8_t *base = (uint8_t*)block + sizeof(block_t);
        *(struct block_fence_t*)base = block->left_fence;
        *(struct block_fence_t*)(base + sizeof(struct block_fence_t) + block->user_size) = block->right_fence;
        update_checksum(block);
        update_checksum_heap(heap);
        pthread_mutex_unlock(&mutex);
        return (void*)block + sizeof(block_t) + sizeof(struct block_fence_t);
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}
void* heap_calloc_aligned(size_t number, size_t size){
    if(number<=0 || size<=0)
        return NULL;
    size_t n_size = number * size;
    if(size != n_size / number) //overflow
        return NULL;
    void * block = heap_malloc_aligned(n_size);
    if(!block)
        return NULL;
    memset(block, 0, n_size);
    return block;
}
void* heap_realloc_aligned(void* memblock, size_t size){
    if(memblock && !size){
        heap_free(memblock);
        return NULL;
    }
    if(!memblock || size<=0)
        return heap_malloc_aligned(size);
    block_t *block = (block_t*)memblock -sizeof(struct block_fence_t) - sizeof(block_t);
    if(block->size >= size){
        block->user_size = size;
        uint8_t *base = (uint8_t*)block + sizeof(block_t);
        *(struct block_fence_t*)(base + sizeof(struct block_fence_t) + size) = block->right_fence;
        update_checksum(block);
        return memblock;
    }
    void *ptr = heap_malloc_aligned(size);
    if(ptr){
        memcpy(ptr,memblock,block->size);
        heap_free(memblock);
    }
    return ptr;
}

void* heap_malloc_aligned_debug(size_t count, int fileline, const char* filename){
    if(fileline < 0 || !filename)
        return NULL;
    void * ptr = heap_malloc_aligned(count);
    if(!ptr)
        return NULL;
    pthread_mutex_lock(&mutex);
    block_t* block = ptr-sizeof(block_t)-sizeof(struct block_fence_t);
    block->filename = filename;
    block->line = fileline;
    update_checksum(block);
    pthread_mutex_unlock(&mutex);
    return ptr;
}
void* heap_calloc_aligned_debug(size_t number, size_t size, int fileline, const char* filename){
    if(fileline < 0 || !filename)
        return NULL;
    void * ptr = heap_calloc_aligned(number,size);
    if(!ptr)
        return NULL;
    pthread_mutex_lock(&mutex);
    block_t* block = ptr-sizeof(block_t)-sizeof(struct block_fence_t);
    block->filename = filename;
    block->line = fileline;
    update_checksum(block);
    pthread_mutex_unlock(&mutex);
    return ptr;
}
void* heap_realloc_aligned_debug(void* memblock, size_t size, int fileline, const char* filename){
     if(fileline < 0 || !filename)
        return NULL;
    void * ptr = heap_realloc_aligned(memblock,size);
    if(!ptr)
        return NULL;
    pthread_mutex_lock(&mutex);
    block_t* block = ptr-sizeof(block_t)-sizeof(struct block_fence_t);
    block->filename = filename;
    block->line = fileline;
    update_checksum(block);
    pthread_mutex_unlock(&mutex);
    return ptr;
}

size_t heap_get_used_space(void){
    if(heap_validate() != 0)
        exit(1);
    return heap->brk-heap->start_brk;
}
size_t heap_get_largest_used_block_size(void){
    if(heap_validate() != 0)
        exit(1);
    if(heap_get_used_blocks_count() == 0)
        return 0;
    pthread_mutex_lock(&mutex);
    block_t *block = heap->first->next;
    size_t size = 0;
    while(block != heap->last){
        if(block->size>size)
            size = block->size;
        block = block->next;
    }
    pthread_mutex_unlock(&mutex);
    return size;
}
uint64_t heap_get_used_blocks_count(void){
    if(heap_validate() != 0)
        exit(1);
    pthread_mutex_lock(&mutex);
    block_t *block = heap->first->next;
    uint64_t count = 0;
    while(block != heap->last){
        if(block->size>0)
            ++count;
        block = block->next;
    }
    pthread_mutex_unlock(&mutex);
    return count;
}
size_t heap_get_free_space(void){
    if(heap_validate() != 0)
        exit(1);
    pthread_mutex_lock(&mutex);
    block_t *block = heap->first->next;
    size_t size=0;
    while(block != heap->last){
        if(block->size < 0)
            size += -block->size;
        block = block->next;
    }
    pthread_mutex_unlock(&mutex);
    return size;
}
size_t heap_get_largest_free_area(void){
    if(heap_validate() != 0)
        exit(1);
    if(heap_get_used_blocks_count() == 0)
        return heap_get_free_space();
    pthread_mutex_lock(&mutex);
    block_t *block = heap->first->next;
    size_t size = 0;
    ptrdiff_t temp = 0;
    while(block != heap->last){
        if(block->size<temp)
            temp = block->size;
        block = block->next;
    }
    temp *=-1;
    size = temp;
    pthread_mutex_unlock(&mutex);
    return size;
}
uint64_t heap_get_free_gaps_count(void){
    if(heap_validate() != 0)
        exit(1);
    pthread_mutex_lock(&mutex);
    block_t *block = heap->first->next;
    uint64_t count = 0;
    while(block != heap->last){
        if(block->size<0)
            count += 1;
        block = block->next;
    }
    pthread_mutex_unlock(&mutex);
    return count;
}

int heap_validate(void){
    pthread_mutex_lock(&mutex);
    // test sumy kontrolnej HEAP
    heap_t temp = *heap;
    temp.checksum = 0;
    size_t chk = calc_checksum(&temp,sizeof(heap_t));
    if(chk != heap->checksum){
        pthread_mutex_unlock(&mutex);
        return 1;   // błędna suma kontrolna
    }
        
    // testy bloków pamięci
    for(block_t *block = heap->first; block != NULL; block = block->next){
        if(block_validate(block) == 1){ // uszkodzony blok pamięci
            pthread_mutex_unlock(&mutex);
            return 1;
        }
    }
    
    pthread_mutex_unlock(&mutex);
    
    return 0; // wszystko wydaje się być w porządku
}

size_t calc_checksum(const void* buffer, size_t size)
{
	size_t chk = 0;
	const unsigned char* ptr = buffer;
	for(int i=0;i<size;++i){
        chk += *(ptr+i);
    }
	return chk;
}

void update_checksum(block_t* block)
{
	block->checksum = 0;
	block->checksum = calc_checksum(block, sizeof(block_t));
}
void update_checksum_heap(heap_t* heap_s)
{
	heap_s->checksum = 0;
	heap_s->checksum = calc_checksum(heap_s, sizeof(heap_t));
}

int block_validate(const block_t* block){
    // test sumy kontrolnej bloku pamięci
    block_t temp = *block;
    temp.checksum = 0;
    size_t chk = calc_checksum(&temp, sizeof(block_t));
    if(chk != block->checksum)
        return 1;
    
    // sprawdzanie płotków tylko w zajętych blokach 
    if(block->size > 0 && block->user_size > 0){  
        // test naruszenia płotków
        struct block_fence_t* left_fence = (struct block_fence_t*)((uint8_t*)block + sizeof(block_t));
        struct block_fence_t* right_fence = (struct block_fence_t*)((uint8_t*)block + sizeof(block_t)+sizeof(struct block_fence_t) + block->user_size);
            
        if(memcmp(left_fence, &block->left_fence,sizeof(struct block_fence_t)) != 0)
            return 1; // uszkodzony płotek na początku bloku
        if(memcmp(right_fence,&block->right_fence,sizeof(struct block_fence_t)) != 0)
            return 1; // uszkodzony płotek na końcu bloku
    }

    return 0; // wszystko wydaje się być w porządku
}

enum pointer_type_t get_pointer_type(const void* pointer){
    if(heap_validate() != 0)
        exit(1);
    pthread_mutex_lock(&mutex);
    if(!pointer){
        pthread_mutex_unlock(&mutex);
        return pointer_null;
    }
    if((intptr_t)pointer >= (intptr_t)heap && (intptr_t)pointer < (intptr_t)heap->first){
        pthread_mutex_unlock(&mutex);
        return pointer_control_block;
    }
    for(block_t *block = heap->first; block != NULL; block = block->next){
        if((intptr_t)pointer >= (intptr_t)block->next)
            continue;
        if(block->size == 0){
            if((intptr_t)pointer >= (intptr_t)block && (intptr_t)pointer < (intptr_t)block+sizeof(block_t)){
                pthread_mutex_unlock(&mutex);
                return pointer_control_block;
            }
        }
        else if(block->size < 0){ //wolny
            if((intptr_t)pointer >= (intptr_t)block && (intptr_t)pointer < (intptr_t)block+sizeof(block_t)+sizeof(struct block_fence_t)){
                pthread_mutex_unlock(&mutex);
                return pointer_control_block;
            }
            if((intptr_t)pointer >= (intptr_t)block + sizeof(block_t)+sizeof(struct block_fence_t) - block->size && (intptr_t)pointer < (intptr_t)block + sizeof(block_t)+2*sizeof(struct block_fence_t) - block->size){
                pthread_mutex_unlock(&mutex);
                return pointer_control_block;
            }
            if((intptr_t)pointer >= (intptr_t)block + sizeof(block_t)+sizeof(struct block_fence_t) && (intptr_t)pointer < (intptr_t)block+sizeof(block_t)+sizeof(struct block_fence_t) - block->size){
                pthread_mutex_unlock(&mutex);
                return pointer_unallocated;
            }
        }
        else if(block->size > 0){ //zajęty
            if((intptr_t)pointer >= (intptr_t)block && (intptr_t)pointer < (intptr_t)block+sizeof(block_t)+sizeof(struct block_fence_t)){
                pthread_mutex_unlock(&mutex);
                return pointer_control_block;
            }
            if((intptr_t)pointer == (intptr_t)block + sizeof(block_t)+sizeof(struct block_fence_t)){
                pthread_mutex_unlock(&mutex);
                return pointer_valid;
            }
            if((intptr_t)pointer > (intptr_t)block + sizeof(block_t)+sizeof(struct block_fence_t) && (intptr_t)pointer < (intptr_t)block + sizeof(block_t)+sizeof(struct block_fence_t) + block->user_size){
                pthread_mutex_unlock(&mutex);
                return pointer_inside_data_block;
            }
            if((intptr_t)pointer >= (intptr_t)block + sizeof(block_t)+sizeof(struct block_fence_t) + block->user_size && (intptr_t)pointer < (intptr_t)block + sizeof(block_t)+2*sizeof(struct block_fence_t) + block->user_size){
                pthread_mutex_unlock(&mutex);
                return pointer_control_block;
            }
            if((intptr_t)pointer >= (intptr_t)block + sizeof(block_t)+2*sizeof(struct block_fence_t) + block->user_size && (intptr_t)pointer < (intptr_t)block + sizeof(block_t)+2*sizeof(struct block_fence_t) + block->size){
                pthread_mutex_unlock(&mutex);
                return pointer_inside_data_block;
            }
        }
    }
    pthread_mutex_unlock(&mutex);
    return pointer_out_of_heap;
}

void* heap_get_data_block_start(const void* pointer){
    if(heap_validate() != 0)
        exit(1);
    enum pointer_type_t type = get_pointer_type(pointer);
    pthread_mutex_lock(&mutex);
    if(type == pointer_valid){
        pthread_mutex_unlock(&mutex);
        return (void*)pointer;
    }
    if(type == pointer_inside_data_block){
        pthread_mutex_unlock(&mutex);
        while(get_pointer_type(--pointer) != pointer_valid);
        return (void*)pointer;
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

size_t heap_get_block_size(const void* memblock){
    if(heap_validate() != 0)
        exit(1);
    if(get_pointer_type(memblock) == pointer_valid){
        const block_t*block = memblock -sizeof(block_t)-sizeof(struct block_fence_t);
        return block->user_size;
    }
    return 0;
}

void heap_dump_debug_information(void){
    if(heap_validate() != 0)
        exit(1);
    pthread_mutex_lock(&mutex);
    printf("------- Informations about blocks: -------\n");
    for(block_t *block = heap->first; block != NULL; block = block->next){
        printf("  Address of block: %p\n",block);
        printf("  Size of block: %ld[B]\n",block->size);
        if(block->filename != NULL){
            printf("  Filename: %s\n",block->filename);
            printf("  Line number: %ld\n",block->line);
        }
        printf("------------------------------------------\n");
    }
    pthread_mutex_unlock(&mutex);
    printf("\n------- Informations about Heap: -------\n");
    printf("  Size of heap: %ld[B]\n",heap_get_used_space());
    printf("  Used space: %ld[B]\n",heap_get_used_space() - heap_get_free_space());
    printf("  Free space: %ld[B]\n",heap_get_free_space());
    printf("  Largest free area: %ld[B]\n",heap_get_largest_free_area());
    printf("----------------------------------------\n");
     
}