#include <stdio.h>
#include "custom_unistd.h"
#include "functions.h"

#if 1  
int main(int argc, char **argv)
{
    /* testy malloc */
    int status = heap_setup();
    assert(status == 0);

    // parametry pustej sterty
    size_t free_bytes = heap_get_free_space();
    size_t used_bytes = heap_get_used_space();

    void* p1 = heap_malloc(8 * 1024 * 1024);  //8MB
    void* p2 = heap_malloc(8 * 1024 * 1024); // 8MB
    void* p3 = heap_malloc(8 * 1024 * 1024); // 8MB
    void* p4 = heap_malloc(45 * 1024 * 1024); // 45MB
    assert(p1 != NULL); // malloc musi się udać
    assert(p2 != NULL); // malloc musi się udać
    assert(p3 != NULL); // malloc musi się udać
    assert(p4 == NULL); // nie ma prawa zadziałać
    // Ostatnia alokacja, na 45MB nie może się powieść,
    // ponieważ sterta nie może być aż tak
    // wielka (brak pamięci w systemie operacyjnym).

    status = heap_validate();
    assert(status == 0); // sterta nie może być uszkodzona


    // zaalokowano 3 bloki
    assert(heap_get_used_blocks_count() == 3);

    // zwolnij pamięć
    heap_free(p1);
    heap_free(p2);
    heap_free(p3);

    // już nie ma bloków
    assert(heap_get_used_blocks_count() == 0);

    /* testy calloc */
    p1 = heap_calloc(8, 1024 * 1024); // 8MB
    p2 = heap_calloc(8, 1024 * 1024); // 8MB
    p3 = heap_calloc(8, 1024 * 1024); // 8MB
    p4 = heap_calloc(45, 1024 * 1024); // 45MB
    assert(p1 != NULL); // calloc musi się udać
    assert(p2 != NULL); // calloc musi się udać
    assert(p3 != NULL); // calloc musi się udać
    assert(p4 == NULL); // nie ma prawa zadziałać
    // Ostatnia alokacja, na 45MB nie może się powieść,
    // ponieważ sterta nie może być aż tak
    // wielka (brak pamięci w systemie operacyjnym).

    status = heap_validate();
    assert(status == 0); // sterta nie może być uszkodzona

    // zaalokowano 3 bloki
    assert(heap_get_used_blocks_count() == 3);

    // zwolnij pamięć
    heap_free(p1);
    heap_free(p2);
    heap_free(p3);

    // test zerowania pamięci
    p1 = heap_calloc(8, 1024 * 1024);
    p2 = heap_calloc(8, 1024 * 1024);
    memset(p1, 1, 8 * 1024 * 1024);
    heap_free(p1);
    p1 = heap_calloc(8, 1024 * 1024);
    assert(memcmp(p1,p2,8 * 1024 * 1024) == 0);
    heap_free(p1);
    heap_free(p2);

    // wszystko powinno wrócić do normy
    assert(heap_get_free_space() == free_bytes);
    assert(heap_get_used_space() == used_bytes);

    // już nie ma bloków
    assert(heap_get_used_blocks_count() == 0);

    /* testy realloc */
    p1 = heap_realloc(NULL, 8* 1024 * 1024); // realloc działa jak malloc
    p2 = heap_realloc(NULL, 8* 1024 * 1024); // realloc działa jak malloc
    p3 = heap_realloc(NULL, 8* 1024 * 1024); // realloc działa jak malloc
    p4 = heap_realloc(p2, 45* 1024 * 1024); // 45 MB
    assert(p1 != NULL); // realloc musi się udać
    assert(p2 != NULL); // realloc musi się udać
    assert(p3 != NULL); // realloc musi się udać
    assert(p4 == NULL); // nie ma prawa zadziałać
    // Ostatnia alokacja, na 45MB nie może się powieść,
    // ponieważ sterta nie może być aż tak
    // wielka (brak pamięci w systemie operacyjnym).

    memset(p1, 1, 8* 1024 * 1024);
    memset(p2, 1, 8* 1024 * 1024);
    void * p5 = heap_realloc(p2, 16 * 1024 * 1024); //realloc zwalnia p2
    assert(p5 != NULL); // realloc musi się udać
    assert(memcmp(p5, p1, 8* 1024 * 1024) == 0); //realloc musi przekopiować dane

    status = heap_validate();
    assert(status == 0); // sterta nie może być uszkodzona

    // zwolnij pamięć
    heap_realloc(p1, 0); //realloc działa jak free
    heap_realloc(p3, 0); //realloc działa jak free
    heap_realloc(p5, 0); //realloc działa jak free

    //gdy w bloku jest tyle pamięci ile potrzeba na realloc lub więcej przesuwany jest płotek i zwracany jest adres powiększanego bloku
    p1 = heap_realloc(NULL,8 * 1024 * 1024); // 8MB
    p2 = heap_realloc(p1,8 * 1024 * 1024); // 8MB
    assert(p1==p2);

    heap_realloc(p1, 0); //realloc działa jak free

    // wszystko powinno wrócić do normy
    assert(heap_get_free_space() == free_bytes);
    assert(heap_get_used_space() == used_bytes);

    // już nie ma bloków
    assert(heap_get_used_blocks_count() == 0);

    /* testy malloc_debug */
    p1 = heap_malloc_debug(8 * 1024 * 1024,__LINE__,__FILE__);  //8MB
    p2 = heap_malloc_debug(8 * 1024 * 1024,__LINE__,__FILE__); // 8MB
    p3 = heap_malloc_debug(8 * 1024 * 1024,__LINE__,__FILE__); // 8MB
    p4 = heap_malloc_debug(45 * 1024 * 1024,__LINE__,__FILE__); // 45MB
    assert(p1 != NULL); // malloc musi się udać
    assert(p2 != NULL); // malloc musi się udać
    assert(p3 != NULL); // malloc musi się udać
    assert(p4 == NULL); // nie ma prawa zadziałać
    // Ostatnia alokacja, na 45MB nie może się powieść,
    // ponieważ sterta nie może być aż tak
    // wielka (brak pamięci w systemie operacyjnym).

    status = heap_validate();
    assert(status == 0); // sterta nie może być uszkodzona


    // zaalokowano 3 bloki
    assert(heap_get_used_blocks_count() == 3);

    // zwolnij pamięć
    heap_free(p1);
    heap_free(p2);
    heap_free(p3);

    // już nie ma bloków
    assert(heap_get_used_blocks_count() == 0);

    /* testy calloc_debug */
    p1 = heap_calloc_debug(8, 1024 * 1024,__LINE__,__FILE__); // 8MB
    p2 = heap_calloc_debug(8, 1024 * 1024,__LINE__,__FILE__); // 8MB
    p3 = heap_calloc_debug(8, 1024 * 1024,__LINE__,__FILE__); // 8MB
    p4 = heap_calloc_debug(45, 1024 * 1024,__LINE__,__FILE__); // 45MB
    assert(p1 != NULL); // calloc musi się udać
    assert(p2 != NULL); // calloc musi się udać
    assert(p3 != NULL); // calloc musi się udać
    assert(p4 == NULL); // nie ma prawa zadziałać
    // Ostatnia alokacja, na 45MB nie może się powieść,
    // ponieważ sterta nie może być aż tak
    // wielka (brak pamięci w systemie operacyjnym).

    status = heap_validate();
    assert(status == 0); // sterta nie może być uszkodzona

    // zaalokowano 3 bloki
    assert(heap_get_used_blocks_count() == 3);

    // zwolnij pamięć
    heap_free(p1);
    heap_free(p2);
    heap_free(p3);

    // test zerowania pamięci
    p1 = heap_calloc_debug(8, 1024 * 1024,__LINE__,__FILE__);
    p2 = heap_calloc_debug(8, 1024 * 1024,__LINE__,__FILE__);
    memset(p1, 1, 8 * 1024 * 1024);
    heap_free(p1);
    p1 = heap_calloc_debug(8, 1024 * 1024,__LINE__,__FILE__);
    assert(memcmp(p1,p2,8 * 1024 * 1024) == 0);
    heap_free(p1);
    heap_free(p2);

    // wszystko powinno wrócić do normy
    assert(heap_get_free_space() == free_bytes);
    assert(heap_get_used_space() == used_bytes);

    // już nie ma bloków
    assert(heap_get_used_blocks_count() == 0);

    /* testy realloc_debug */
    p1 = heap_realloc_debug(NULL, 8* 1024 * 1024,__LINE__,__FILE__); // realloc działa jak malloc
    p2 = heap_realloc_debug(NULL, 8* 1024 * 1024,__LINE__,__FILE__); // realloc działa jak malloc
    p3 = heap_realloc_debug(NULL, 8* 1024 * 1024,__LINE__,__FILE__); // realloc działa jak malloc
    p4 = heap_realloc_debug(p2, 45* 1024 * 1024,__LINE__,__FILE__); // 45MB
    assert(p1 != NULL); // realloc musi się udać
    assert(p2 != NULL); // realloc musi się udać
    assert(p3 != NULL); // realloc musi się udać
    assert(p4 == NULL); // nie ma prawa zadziałać
    // Ostatnia alokacja, na 45MB nie może się powieść,
    // ponieważ sterta nie może być aż tak
    // wielka (brak pamięci w systemie operacyjnym).

    memset(p1, 1, 8* 1024 * 1024);
    memset(p2, 1, 8* 1024 * 1024);
    p5 = heap_realloc_debug(p2, 16 * 1024 * 1024,__LINE__,__FILE__); //realloc zwalnia p2
    assert(p5 != NULL); // realloc musi się udać
    assert(memcmp(p5, p1, 8* 1024 * 1024) == 0); //realloc musi przekopiować dane

    status = heap_validate();
    assert(status == 0); // sterta nie może być uszkodzona

    // zwolnij pamięć
    heap_realloc_debug(p1, 0,__LINE__,__FILE__); //realloc działa jak free
    heap_realloc_debug(p3, 0,__LINE__,__FILE__); //realloc działa jak free
    heap_realloc_debug(p5, 0,__LINE__,__FILE__); //realloc działa jak free

    //gdy w bloku jest tyle pamięci ile potrzeba na realloc lub więcej przesuwany jest płotek i zwracany jest adres powiększanego bloku
    p1 = heap_realloc_debug(NULL,8 * 1024 * 1024,__LINE__,__FILE__); // 8MB
    p2 = heap_realloc_debug(p1,8 * 1024 * 1024,__LINE__,__FILE__); // 8MB
    assert(p1==p2);

    heap_realloc_debug(p1, 0,__LINE__,__FILE__); //realloc działa jak free

    // wszystko powinno wrócić do normy
    assert(heap_get_free_space() == free_bytes);
    assert(heap_get_used_space() == used_bytes);

    // już nie ma bloków
    assert(heap_get_used_blocks_count() == 0);

    /* testy malloc_aligned */
    p1 = heap_malloc_aligned(8 * 1024 * 1024); // 8MB
    assert(p1 == NULL); // malloc_aligned nie ma prawa zadziałać
    //przygotowanie miejsca na aligned_block
    p1 = heap_malloc(8 * 1024 * 1024); // 8MB
    p2 = heap_malloc(8 * 1024 * 1024); // 8MB
    heap_free(p2);

    p2 = heap_malloc_aligned(1*1024*1024); // 1MB
    assert(((intptr_t)p2 & (intptr_t)(PAGE_SIZE-1)) == 0); // Blok musi być na początku strony pamięci

    // przygotowanie do testu idealnej alokacji
    p3 = heap_malloc(3728); // idealna alokacja malloc
    p4 = heap_malloc(7336128); // idealna alokacja malloc
    heap_free(p2);
    p2 = heap_malloc_aligned(1*1024*1024); // idealna alokacja malloc_aligned
    assert(((intptr_t)p2 & (intptr_t)(PAGE_SIZE-1)) == 0); // Blok musi być na początku strony pamięci
    heap_free(p2);
    p2 = heap_malloc_aligned(1*1024*1024-10); // idelana alokacja i brak miejsca na kolejny blok
    assert(((intptr_t)p2 & (intptr_t)(PAGE_SIZE-1)) == 0); // Blok musi być na początku strony pamięci
    heap_free(p2);
    p2 = heap_malloc_aligned(1*1024*1024-96); // idelana alokacja i jest miejsce na kolejny blok
    assert(((intptr_t)p2 & (intptr_t)(PAGE_SIZE-1)) == 0); // Blok musi być na początku strony pamięci

    status = heap_validate();
    assert(status == 0); // sterta nie może być uszkodzona

    // zaalokowano 4 bloki
    assert(heap_get_used_blocks_count() == 4);

    // zwolnij pamięć
    heap_free(p1);
    heap_free(p2);
    heap_free(p3);
    heap_free(p4);

    // wszystko powinno wrócić do normy
    assert(heap_get_free_space() == free_bytes);
    assert(heap_get_used_space() == used_bytes);

    // już nie ma bloków
    assert(heap_get_used_blocks_count() == 0);
    
    /* testy calloc_aligned */
    p1 = heap_calloc_aligned(8, 1024 * 1024); // 8MB
    assert(p1 == NULL); // calloc_aligned nie ma prawa zadziałać
    //przygotowanie miejsca na aligned_block
    p1 = heap_malloc(8 * 1024 * 1024); // 8MB
    p2 = heap_malloc(8 * 1024 * 1024); // 8MB
    heap_free(p2);

    p2 = heap_calloc_aligned(1,1024*1024); // 1MB
    assert(((intptr_t)p2 & (intptr_t)(PAGE_SIZE-1)) == 0); // Blok musi być na początku strony pamięci

    // przygotowanie do testu idealnej alokacji
    p3 = heap_malloc(3728); // idealna alokacja malloc
    p4 = heap_malloc(7336128); // idealna alokacja malloc
    heap_free(p2);
    p2 = heap_calloc_aligned(1,1024*1024); // idealna alokacja calloc_aligned
    assert(((intptr_t)p2 & (intptr_t)(PAGE_SIZE-1)) == 0); // Blok musi być na początku strony pamięci
    heap_free(p2);
    p2 = heap_calloc_aligned(1,1024*1024-10); // idelana alokacja i brak miejsca na kolejny blok
    assert(((intptr_t)p2 & (intptr_t)(PAGE_SIZE-1)) == 0); // Blok musi być na początku strony pamięci
    heap_free(p2);
    p2 = heap_calloc_aligned(1,1024*1024-96); // idelana alokacja i jest miejsce na kolejny blok
    assert(((intptr_t)p2 & (intptr_t)(PAGE_SIZE-1)) == 0); // Blok musi być na początku strony pamięci
    heap_free(p2);
    
    // test zerowania pamięci
    p2 = heap_calloc_aligned(1,1024*1024); 
    heap_free(p4);
    p5 = heap_calloc_aligned(1,1024*1024);
    memset(p2, 1, 1024*1024);
    heap_free(p2);
    p2 = heap_calloc_aligned(1,1024*1024); 
    assert(memcmp(p2,p5, 1024*1024) == 0);
    
    status = heap_validate();
    assert(status == 0); // sterta nie może być uszkodzona

    // zaalokowano 4 bloki
    assert(heap_get_used_blocks_count() == 4);

    // zwolnij pamięć
    heap_free(p1);
    heap_free(p2);
    heap_free(p3);
    heap_free(p5);

    // wszystko powinno wrócić do normy
    assert(heap_get_free_space() == free_bytes);
    assert(heap_get_used_space() == used_bytes);

    // już nie ma bloków
    assert(heap_get_used_blocks_count() == 0);
    
    /* testy realloc_aligned */
    p1 = heap_realloc_aligned(NULL,8 * 1024 * 1024); // 8MB
    assert(p1 == NULL); // realloc_aligned nie ma prawa zadziałać
    //przygotowanie miejsca na aligned_block
    p1 = heap_malloc(8 * 1024 * 1024); // 8MB
    p2 = heap_malloc(8 * 1024 * 1024); // 8MB
    heap_free(p2);

    p2 = heap_realloc_aligned(NULL,1*1024*1024); // 1MB
    assert(((intptr_t)p2 & (intptr_t)(PAGE_SIZE-1)) == 0); // Blok musi być na początku strony pamięci
    // przygotowanie do testu idealnej alokacji
    p3 = heap_malloc(3728); // idealna alokacja malloc
    p4 = heap_malloc(7336128); // idealna alokacja malloc
    heap_free(p2);
    p2 = heap_realloc_aligned(NULL,1*1024*1024); // idealna alokacja realloc_aligned
    assert(((intptr_t)p2 & (intptr_t)(PAGE_SIZE-1)) == 0); // Blok musi być na początku strony pamięci
    heap_free(p2);
    p2 = heap_realloc_aligned(NULL,1*1024*1024-10); // idelana alokacja i brak miejsca na kolejny blok
    assert(((intptr_t)p2 & (intptr_t)(PAGE_SIZE-1)) == 0); // Blok musi być na początku strony pamięci
    heap_free(p2);
    p2 = heap_realloc_aligned(NULL,1*1024*1024-96); // idelana alokacja i jest miejsce na kolejny blok
    assert(((intptr_t)p2 & (intptr_t)(PAGE_SIZE-1)) == 0); // Blok musi być na początku strony pamięci
    heap_realloc_aligned(p2,0); //realloc działa jak free
    
    p2 = heap_realloc_aligned(NULL,1024*1024-10); 
    assert(p2 != NULL);
    memset(p2, 1, 1024*1024-10);
    p5 = heap_realloc_aligned(p2,1024*1024);
    assert(p2==p5); //gdy w bloku jest tyle pamięci ile potrzeba na realloc lub więcej przesuwany jest płotek i zwracany jest adres powiększanego bloku

    status = heap_validate();
    assert(status == 0); // sterta nie może być uszkodzona

    // zaalokowano 4 bloki
    assert(heap_get_used_blocks_count() == 4);

    // zwolnij pamięć
    heap_realloc_aligned(p1,0);
    heap_realloc_aligned(p2,0);
    heap_realloc_aligned(p3,0);
    heap_realloc_aligned(p4,0);

    // wszystko powinno wrócić do normy
    assert(heap_get_free_space() == free_bytes);
    assert(heap_get_used_space() == used_bytes);

    // już nie ma bloków
    assert(heap_get_used_blocks_count() == 0);
    
    /* testy pointer_type */
    p1 = heap_malloc(100);
    assert(heap_get_block_size(p1) == 100); //rozmiar bloku musi się zgadzać 
    assert(get_pointer_type(p1) == pointer_valid); // wskaźnik musi być typu pointer_valid
    assert(get_pointer_type(NULL) == pointer_null); // wskaźnik musi być typu pointer_null
    assert(get_pointer_type(p1-1) == pointer_control_block); // wskaźnik musi być typu pointer_control_block
    assert(get_pointer_type((void*)500) == pointer_out_of_heap); // wskaźnik musi być typu pointer_out_of_heap
    assert(get_pointer_type(p1+sizeof(struct block_fence_t)) == pointer_inside_data_block); // wskaźnik musi być typu pointer_inside_data_block
    assert(get_pointer_type(p1+2*sizeof(struct block_fence_t)+sizeof(block_t)+110) == pointer_unallocated); // wskaźnik musi być typu pointer_unallocated

    // zaalokowano 1 blok
    assert(heap_get_used_blocks_count() == 1);

    // zwolnij pamięć
    heap_free(p1);

    // już nie ma bloków
    assert(heap_get_used_blocks_count() == 0);
    
    printf("Testy zakończone pomyślnie! :)\n");
    return 0;
}
#endif

#if 0
#include "custom_unistd.h"

int main(int argc, char **argv)
{
    int status = heap_setup();
    assert(status == 0);

    // parametry pustej sterty
    size_t free_bytes = heap_get_free_space();
    size_t used_bytes = heap_get_used_space();

    void* p1 = heap_malloc(8 * 1024 * 1024); // 8MB
    void* p2 = heap_malloc(8 * 1024 * 1024); // 8MB
    void* p3 = heap_malloc(8 * 1024 * 1024); // 8MB
    void* p4 = heap_malloc(45 * 1024 * 1024); // 45MB
    assert(p1 != NULL); // malloc musi się udać
    assert(p2 != NULL); // malloc musi się udać
    assert(p3 != NULL); // malloc musi się udać
    assert(p4 == NULL); // nie ma prawa zadziałać
    // Ostatnia alokacja, na 45MB nie może się powieść,
    // ponieważ sterta nie może być aż tak
    // wielka (brak pamięci w systemie operacyjnym).

    status = heap_validate();
    assert(status == 0); // sterta nie może być uszkodzona


    // zaalokowano 3 bloki
    assert(heap_get_used_blocks_count() == 3);

    // zajęto 24MB sterty; te 2000 bajtów powinno
    // wystarczyć na wewnętrzne struktury sterty
    assert(
        heap_get_used_space() >= 24 * 1024 * 1024 &&
        heap_get_used_space() <= 24 * 1024 * 1024 + 2000
    );

    // zwolnij pamięć
    heap_free(p1);
    heap_free(p2);
    heap_free(p3);

    // wszystko powinno wrócić do normy

    assert(heap_get_free_space() == free_bytes);
    assert(heap_get_used_space() == used_bytes);

    // już nie ma bloków
    assert(heap_get_used_blocks_count() == 0);

    return 0;
}
#endif
