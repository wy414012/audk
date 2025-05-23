/** @file
  Support routines for memory allocation routines based on Standalone MM Core internal functions.

  Copyright (c) 2015 - 2025, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2016 - 2021, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>

#include <Guid/MmramMemoryReserve.h>
#include <Library/PhaseMemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include "StandaloneMmCoreMemoryAllocationServices.h"

GLOBAL_REMOVE_IF_UNREFERENCED CONST EFI_MEMORY_TYPE gPhaseDefaultDataType = EfiRuntimeServicesData;
GLOBAL_REMOVE_IF_UNREFERENCED CONST EFI_MEMORY_TYPE gPhaseDefaultCodeType = EfiRuntimeServicesCode;

static EFI_MM_SYSTEM_TABLE  *mMemoryAllocationMmst = NULL;

/**
  Allocates one or more 4KB pages of a certain memory type.

  Allocates the number of 4KB pages of a certain memory type and returns a pointer
  to the allocated buffer.  The buffer returned is aligned on a 4KB boundary.  If
  Pages is 0, then NULL is returned.   If there is not enough memory remaining to
  satisfy the request, then NULL is returned.

  @param  Type                  The type of allocation to perform.
  @param  MemoryType            The type of memory to allocate.
  @param  Pages                 The number of 4 KB pages to allocate.
  @param  Memory                The pointer to a physical address. On input, the
                                way in which the address is used depends on the
                                value of Type.

  @retval EFI_SUCCESS           The requested pages were allocated.
  @retval EFI_INVALID_PARAMETER 1) Type is not AllocateAnyPages or
                                AllocateMaxAddress or AllocateAddress.
                                2) MemoryType is in the range
                                EfiMaxMemoryType..0x6FFFFFFF.
                                3) Memory is NULL.
                                4) MemoryType is EfiPersistentMemory.
  @retval EFI_OUT_OF_RESOURCES  The pages could not be allocated.
  @retval EFI_NOT_FOUND         The requested pages could not be found.

**/
EFI_STATUS
EFIAPI
PhaseAllocatePages (
  IN     EFI_ALLOCATE_TYPE     Type,
  IN     EFI_MEMORY_TYPE       MemoryType,
  IN     UINTN                 Pages,
  IN OUT EFI_PHYSICAL_ADDRESS  *Memory
  )
{
  if (Pages == 0) {
    return EFI_INVALID_PARAMETER;
  }

  return mMemoryAllocationMmst->MmAllocatePages (Type, MemoryType, Pages, Memory);
}

/**
  Frees one or more 4KB pages that were previously allocated with one of the page allocation
  functions in the Memory Allocation Library.

  Frees the number of 4KB pages specified by Pages from the buffer specified by Buffer.
  Buffer must have been allocated on a previous call to the page allocation services
  of the Memory Allocation Library.  If it is not possible to free allocated pages,
  then this function will perform no actions.

  If Buffer was not allocated with a page allocation function in the Memory Allocation
  Library, then ASSERT().
  If Pages is zero, then ASSERT().

  @param  Memory                The base physical address of the pages to be freed.
  @param  Pages                 The number of 4 KB pages to free.

  @retval EFI_SUCCESS           The requested pages were freed.
  @retval EFI_NOT_FOUND         The requested memory pages were not allocated with
                                PhaseAllocatePages().

**/
EFI_STATUS
EFIAPI
PhaseFreePages (
  IN EFI_PHYSICAL_ADDRESS  Memory,
  IN UINTN                 Pages
  )
{
  ASSERT (Pages != 0);
  return mMemoryAllocationMmst->MmFreePages (Memory, Pages);
}

/**
  Allocates a buffer of a certain pool type.

  Allocates the number bytes specified by AllocationSize of a certain pool type and returns a
  pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is
  returned.  If there is not enough memory remaining to satisfy the request, then NULL is returned.

  @param  MemoryType            The type of memory to allocate.
  @param  AllocationSize        The number of bytes to allocate.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
PhaseAllocatePool (
  IN EFI_MEMORY_TYPE  MemoryType,
  IN UINTN            AllocationSize
  )
{
  EFI_STATUS  Status;
  VOID        *Memory;

  Memory = NULL;

  Status = mMemoryAllocationMmst->MmAllocatePool (MemoryType, AllocationSize, &Memory);
  if (EFI_ERROR (Status)) {
    Memory = NULL;
  }

  return Memory;
}

/**
  Frees a buffer that was previously allocated with one of the pool allocation functions in the
  Memory Allocation Library.

  Frees the buffer specified by Buffer.  Buffer must have been allocated on a previous call to the
  pool allocation services of the Memory Allocation Library.  If it is not possible to free pool
  resources, then this function will perform no actions.

  If Buffer was not allocated with a pool allocation function in the Memory Allocation Library,
  then ASSERT().

  @param  Buffer                Pointer to the buffer to free.

**/
VOID
EFIAPI
PhaseFreePool (
  IN VOID  *Buffer
  )
{
  EFI_STATUS  Status;

  Status = mMemoryAllocationMmst->MmFreePool (Buffer);
  ASSERT_EFI_ERROR (Status);
}

/**
  The constructor function calls MmInitializeMemoryServices to initialize
  memory in MMRAM and caches EFI_MM_SYSTEM_TABLE pointer.

  @param  [in]  ImageHandle     The firmware allocated handle for the EFI image.
  @param  [in]  MmSystemTable   A pointer to the Management mode System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
MemoryAllocationLibConstructor (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  )
{
  VOID                            *HobStart;
  EFI_MMRAM_HOB_DESCRIPTOR_BLOCK  *MmramRangesHobData;
  EFI_MMRAM_DESCRIPTOR            *MmramRanges;
  UINTN                           MmramRangeCount;
  EFI_HOB_GUID_TYPE               *MmramRangesHob;

  HobStart = GetHobList ();
  DEBUG ((DEBUG_INFO, "StandaloneMmCoreMemoryAllocationLibConstructor - 0x%x\n", HobStart));

  //
  // Search for a Hob containing the MMRAM ranges
  //
  MmramRangesHob = GetNextGuidHob (&gEfiSmmSmramMemoryGuid, HobStart);
  if (MmramRangesHob == NULL) {
    MmramRangesHob = GetNextGuidHob (&gEfiMmPeiMmramMemoryReserveGuid, HobStart);
    if (MmramRangesHob == NULL) {
      return EFI_UNSUPPORTED;
    }
  }

  MmramRangesHobData = GET_GUID_HOB_DATA (MmramRangesHob);
  if (MmramRangesHobData == NULL) {
    return EFI_UNSUPPORTED;
  }

  MmramRanges = MmramRangesHobData->Descriptor;
  if (MmramRanges == NULL) {
    return EFI_UNSUPPORTED;
  }

  MmramRangeCount = (UINTN)MmramRangesHobData->NumberOfMmReservedRegions;
  if (MmramRanges == NULL) {
    return EFI_UNSUPPORTED;
  }

  {
    UINTN  Index;

    DEBUG ((DEBUG_INFO, "MmramRangeCount - 0x%x\n", MmramRangeCount));
    for (Index = 0; Index < MmramRangeCount; Index++) {
      DEBUG ((
        DEBUG_INFO,
        "MmramRanges[%d]: 0x%016lx - 0x%016lx\n",
        Index,
        MmramRanges[Index].CpuStart,
        MmramRanges[Index].PhysicalSize
        ));
    }
  }

  //
  // Initialize memory service using free MMRAM
  //
  DEBUG ((DEBUG_INFO, "MmInitializeMemoryServices\n"));
  MmInitializeMemoryServices ((UINTN)MmramRangeCount, (VOID *)(UINTN)MmramRanges);

  // Initialize MM Services Table
  mMemoryAllocationMmst = MmSystemTable;
  return EFI_SUCCESS;
}
