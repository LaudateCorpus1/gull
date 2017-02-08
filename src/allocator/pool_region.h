/*
 *  (c) Copyright 2016-2017 Hewlett Packard Enterprise Development Company LP.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  As an exception, the copyright holders of this Library grant you permission
 *  to (i) compile an Application with the Library, and (ii) distribute the
 *  Application containing code generated by the Library and added to the
 *  Application during this compilation process under terms of your choice,
 *  provided you also meet the terms and conditions of the Application license.
 *
 */

#ifndef _NVMM_POOL_REGION_H_
#define _NVMM_POOL_REGION_H_

#include <stddef.h>
#include <stdint.h>

#include "nvmm/error_code.h"
#include "nvmm/shelf_id.h"
#include "nvmm/region.h"

#include "shelf_mgmt/pool.h"

namespace nvmm{

class ShelfRegion;
    
class PoolRegion : public Region
{
public:    
    PoolRegion() = delete;
    PoolRegion(PoolId pool_id);
    ~PoolRegion();    
    ErrorCode Create(size_t size);
    ErrorCode Destroy();
    bool Exist();
    
    ErrorCode Open(int flags);
    ErrorCode Close();
    size_t Size()
    {
        return size_;
    }
    bool IsOpen()
    {
        return is_open_;
    }
    
    ErrorCode Map(void *addr_hint, size_t length, int prot, int flags, loff_t offset, void **mapped_addr);
    ErrorCode Unmap(void *mapped_addr, size_t length);

private:
    static int const kShelfIdx = 0; // this is the only shelf in the pool

    PoolId pool_id_;    
    Pool pool_;
    size_t size_;
    ShelfRegion *region_file_;
    bool is_open_;
};
    
} // namespace nvmm
#endif
