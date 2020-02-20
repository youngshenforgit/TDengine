/*
 * Copyright (c) 2019 TAOS Data, Inc. <jhtao@taosdata.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#define _DEFAULT_SOURCE
#include "os.h"
#include "taosdef.h"
#include "tsched.h"
#include "mnode.h"
#include "mgmtAcct.h"
#include "mgmtBalance.h"
#include "mgmtDb.h"
#include "mgmtDnode.h"
#include "mgmtDnodeInt.h"
#include "mgmtVgroup.h"
#include "mgmtUser.h"
#include "mgmtSystem.h"
#include "mgmtTable.h"
#include "mgmtShell.h"
#include "dnodeModule.h"

char tsMgmtDirectory[128] = {0};
void *tsMgmtTmr           = NULL;
void *tsMgmtTranQhandle   = NULL;
void *tsMgmtStatisTimer   = NULL;

void mgmtCleanUpSystem() {
  mPrint("starting to clean up mgmt");

  taosTmrStopA(&tsMgmtStatisTimer);
  mgmtCleanUpRedirect();
  sdbCleanUpPeers();
  mgmtCleanupBalance();
  mgmtCleanUpDnodeInt();
  mgmtCleanUpShell();
  mgmtCleanUpMeters();
  mgmtCleanUpVgroups();
  mgmtCleanUpDbs();
  mgmtCleanUpDnodes();
  mgmtCleanUpUsers();
  mgmtCleanUpAccts();
  taosTmrCleanUp(tsMgmtTmr);
  taosCleanUpScheduler(tsMgmtTranQhandle);

  mPrint("mgmt is cleaned up");
}

int32_t mgmtStartSystem() {
  mPrint("starting to initialize TDengine mgmt ...");

  struct stat dirstat;
  if (stat(tsMgmtDirectory, &dirstat) < 0) {
    mkdir(tsMgmtDirectory, 0755);
  }

  if (mgmtCheckMgmtRunning() != 0) {
    mPrint("TDengine mgmt module already started...");
    return 0;
  }

  tsMgmtTranQhandle = taosInitScheduler(tsMaxDnodes + tsMaxShellConns, 1, "mnodeT");

  tsMgmtTmr = taosTmrInit((tsMaxDnodes + tsMaxShellConns) * 3, 200, 3600000, "MND");
  if (tsMgmtTmr == NULL) {
    mError("failed to init timer, exit");
    return -1;
  }

  if (mgmtInitAccts() < 0) {
    mError("failed to init accts");
    return -1;
  }

  if (mgmtInitUsers() < 0) {
    mError("failed to init users");
    return -1;
  }

  if (mgmtInitDnodes() < 0) {
    mError("failed to init dnodes");
    return -1;
  }

  if (mgmtInitDbs() < 0) {
    mError("failed to init dbs");
    return -1;
  }

  if (mgmtInitVgroups() < 0) {
    mError("failed to init vgroups");
    return -1;
  }

  if (mgmtInitMeters() < 0) {
    mError("failed to init meters");
    return -1;
  }

  if (mgmtInitDnodeInt() < 0) {
    mError("failed to init inter-mgmt communication");
    return -1;
  }

  if (mgmtInitShell() < 0) {
    mError("failed to init shell");
    return -1;
  }

  if (sdbInitPeers(tsMgmtDirectory) < 0) {
    mError("failed to init peers");
    return -1;
  }

  if (mgmtInitBalance() < 0) {
    mError("failed to init dnode balance")
  }

  mgmtCheckAcct();

  taosTmrReset(mgmtDoStatistic, tsStatusInterval * 30000, NULL, tsMgmtTmr, &tsMgmtStatisTimer);

  mPrint("TDengine mgmt is initialized successfully");

  return 0;
}

int32_t mgmtInitSystemImp() {
  int32_t code = mgmtStartSystem();
  if (code != 0) {
    return code;
  }

  taosTmrReset(mgmtProcessDnodeStatus, 500, NULL, tsMgmtTmr, &mgmtStatusTimer);
  return code;
}

int32_t (*mgmtInitSystem)() = mgmtInitSystemImp;

int32_t mgmtCheckMgmtRunningImp() {
  return 0;
}

int32_t (*mgmtCheckMgmtRunning)() = mgmtCheckMgmtRunningImp;

void mgmtDoStatisticImp(void *handle, void *tmrId) {}

void (*mgmtDoStatistic)(void *handle, void *tmrId) = mgmtDoStatisticImp;

void mgmtStopSystemImp() {}

void (*mgmtStopSystem)() = mgmtStopSystemImp;

void mgmtCleanUpRedirectImp() {}

void (*mgmtCleanUpRedirect)() = mgmtCleanUpRedirectImp;