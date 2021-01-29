/*
 *  (c) Copyright 2016-2021 Hewlett Packard Enterprise Development Company LP.
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

#ifndef _NVMM_EPOCH_ZONE_HEAP_H_
#define _NVMM_EPOCH_ZONE_HEAP_H_

#include <condition_variable>
#include <mutex>
#include <stddef.h>
#include <stdint.h>
#include <thread>

#include "nvmm/error_code.h"
#include "nvmm/global_ptr.h"
#include "nvmm/heap.h"
#include "nvmm/shelf_id.h"

#include "shelf_mgmt/pool.h"
#include "shelf_usage/zone_entry_stack.h"

namespace nvmm {

class ShelfHeap;
class ShelfRegion;

struct shelf_size {
    uint64_t headersize;
    uint64_t headeroffset;
    uint64_t shelfsize;
};

enum EpochZoneHeapOps { OP_RESIZE = 1, OP_CHANGE_PERM = 2 };

//
// This header is in shelf zero, used to store global shelf data strcuture.
//
struct GlobalHeader {
    uint64_t op_in_progress;
    uint64_t destroy_in_progress;
    uint64_t fast_alloc;
    uint64_t total_shelfs;
    uint64_t total_size;
    shelf_size sz[ShelfId::kMaxShelfCount];
};

// zone heap with delayed free
// the only difference from the regular zone heap is: we store the 5 global
// lists for delayed free at the beginning of the header shelf
class EpochZoneHeap : public Heap {
  public:
    EpochZoneHeap() = delete;
    EpochZoneHeap(PoolId pool_id);
    ~EpochZoneHeap();

    ErrorCode Create(size_t shelf_size, size_t min_alloc_size,
                     uint64_t fast_alloc,
                     mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    ErrorCode Destroy();
    bool Exist();
    ErrorCode Resize(size_t);
    ErrorCode SetPermission(mode_t mode);
    ErrorCode GetPermission(mode_t *mode);

    ErrorCode Open(int flags = 0);
    ErrorCode Close();
    size_t Size();
    bool IsOpen() { return is_open_; }

    bool IsInvalid() { return is_invalid_; }

    GlobalPtr Alloc(size_t size);
    void Free(GlobalPtr global_ptr);
    ErrorCode Map(Offset offset, size_t size, void *addr_hint, int prot,
                  void **mapped_addr);
    ErrorCode Unmap(Offset offset, void *mapped_addr, size_t size);

    GlobalPtr Alloc(EpochOp &op, size_t size);
    Offset AllocOffset(size_t size);

    void Free(EpochOp &op, GlobalPtr global_ptr);
    void Free(EpochOp &op, Offset offset);
    void Free(Offset offset);

    void *OffsetToLocal(Offset offset);
    void *GlobalToLocal(GlobalPtr global_ptr);
    // TODO: not implemented yet
    GlobalPtr LocalToGlobal(void *addr);

    size_t MinAllocSize();
    void Merge();
    void OnlineRecover();
    void OfflineRecover();
    void Stats();
    void delayed_free_fn();

  private:
    static int const kHeaderIdx = 0; // headers for zone
    static int const kZoneIdx = 1;   // zone

    static int const kListCnt = 5; // 5 global freelists for delayed free
    static uint64_t const kWorkerSleepMicroSeconds = 50000;
    uint64_t kFreeCnt =
        1000; // free up to 1000 chunks everytime the background worker wakes up
    int total_mapped_shelfs_;

    GlobalHeader *gh_;

    PoolId pool_id_;
    Pool pool_;

    size_t rmb_size_[ShelfId::kMaxShelfCount];
    ShelfHeap *rmb_[ShelfId::kMaxShelfCount]; // zone heap

    ShelfRegion *region_; // headers of global freelists + headers of allocated
                          // memory chunks fron zone heap
    void *mapped_addr_[ShelfId::kMaxShelfCount];
    void *header_[ShelfId::kMaxShelfCount];
    void *bitmap_start_[ShelfId::kMaxShelfCount];

    ZoneEntryStack *global_list_[ShelfId::kMaxShelfCount];
    uint64_t min_obj_size_;
    uint64_t fast_alloc_;

    bool is_open_;
    bool is_invalid_;
    int shelf_id_for_create_;
    size_t shelf_size_for_create_;
    size_t header_size_;

    ErrorCode OpenNewShelfs();
    ErrorCode OpenShelf(int shelf_num);
    ErrorCode CloseShelf(int shelf_num);
    size_t get_header_size_from_size(size_t shelf_size, size_t min_alloc_size,
                                     ShelfIndex shelf_idx);
    size_t get_header_shelf_size(int shelf_num);
    size_t get_total_header_size();
    size_t get_total_size();
    int get_total_data_shelfs();
    Offset get_offset_from_shelfIndexoffset(Offset offset);
    int get_shelfnum_from_shelfIndexoffset(Offset offset);

    // for the background cleaner thread
    std::thread cleaner_thread_;
    std::mutex cleaner_mutex_;
    std::condition_variable running_cv_;
    bool no_bgthread_;
    bool cleaner_start_;
    bool cleaner_stop_;
    bool cleaner_running_;

    // start/stop the background cleaner
    int StartWorker();
    int StopWorker();
    void BackgroundWorker();
    void OfflineFree();
};
} // namespace nvmm
#endif
