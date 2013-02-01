#include <assert.h>
#include <stdbool.h>

#include "execute.h"
#include "paging.h"
#include "system_state.h"


bool had_paging = false;


void update_cr(int cri)
{
    switch (cri)
    {
        case 0:
            assert(!(cr[0] & (1 << 3)));
            assert(  cr[0] & (1 << 0) );
            if (had_paging == !!(cr[0] & (1 << 31)))
                break;
            had_paging = !!(cr[0] & (1 << 31));
        case 3:
            full_paging_update();
    }
}
