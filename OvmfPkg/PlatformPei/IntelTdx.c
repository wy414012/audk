/** @file
  Initialize Intel TDX support.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <IndustryStandard/Tdx.h>
#include <IndustryStandard/QemuFwCfg.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/TdxLib.h>
#include <Library/TdxHelperLib.h>
#include <Library/PlatformInitLib.h>
#include <WorkArea.h>
#include <ConfidentialComputingGuestAttr.h>
#include "Platform.h"

/**
  This Function checks if TDX is available, if present then it sets
  the dynamic PCDs for Tdx guest.
  **/
VOID
IntelTdxInitialize (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
 #ifdef MDE_CPU_X64
  RETURN_STATUS  PcdStatus;
  UINT64         PageMask;

  if (!TdIsEnabled ()) {
    return;
  }

  PcdStatus = PcdSet64S (PcdConfidentialComputingGuestAttr, CCAttrIntelTdx);
  ASSERT_RETURN_ERROR (PcdStatus);

  PlatformInfoHob->PcdConfidentialComputingGuestAttr = CCAttrIntelTdx;

  PageMask  = TdSharedPageMask ();
  PcdStatus = PcdSet64S (PcdTdxSharedBitMask, PageMask);
  ASSERT_RETURN_ERROR (PcdStatus);

  PlatformInfoHob->PcdTdxSharedBitMask = PageMask;
 #endif
}
