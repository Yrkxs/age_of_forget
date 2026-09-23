#include "UsrAI.h"
#include <iostream>
#include <cstdlib>
#include<algorithm>

using namespace std;
tagGame tagUsrGame;
ins UsrIns;
/*##########DO NOT MODIFY THE CODE ABOVE##########*/

#include <cmath>



// 处理数据
// =====================================================================================
//  指令分流 
// 证据1 Core_List.cpp:159 —— addRelation 发现这个单位已有 respond 关系时，会先
//        suspendRelation() 取消旧行动，再建新关系；也就是说"重复下同一条令"不是
//        无害的空操作，而是把这个单位的进度清零重来。
// 证据2 Core.cpp:600-602 —— 引擎作者当年也踩过：AI 每帧重发 HumanAction，
//        suspendRelation 把关系重置，箭塔永远不开火。
// 所以这里统一加两层节流（调用点不用改）：
//   ① 每个单位 3 帧内最多收一条指令；"同一条令"在自己的冷却期内直接丢弃；
//   ② 每帧总指令数上限 32 条；被丢掉的下一帧会重新尝试。
// =====================================================================================
long long UsrAI::ordKeyOf(int type,int obSN,int option,int blockDR,int blockUR,double dr,double ur){
    long long k = (long long)(type & 0xFF) << 56;
    switch(type){
    case INS_HUMANACTION:    return k | (long long)(obSN & 0xFFFFFF);
    case INS_HUMANBUILD:     return k | ((long long)(blockDR & 0xFF) << 16)
                                        | ((long long)(blockUR & 0xFF) << 8)
                                        |  (long long)(option & 0xFF);
    case INS_BUILDINGACTION: return k | (long long)(option & 0xFFFF);
    case INS_CANCEL:         return k;
    case INS_HUMANMOVE: {
        long long a = (long long)(dr * 4.0), b = (long long)(ur * 4.0);
        return k | ((a & 0x7FFFF) << 20) | (b & 0x7FFFF);
    }
    default: return 0;                       // 其它类型不参与重复抑制
    }
}
int UsrAI::ordGapOf(int sn){
    int sort = sn / 10000;
    if(sort == SORT_ARMY)   return 4;        // 军队：反应快一点
    if(sort == SORT_FARMER) return 10;       // 农民：0.4 秒内别改主意
    return 10;                               // 建筑（研发/造兵/升级）
}
bool UsrAI::ordAllow(int sn, long long key){
    int idx = -1;
    for(int i = 0;i < ordN;i++) if(ordSN[i] == sn){ idx = i; break; }
    if(idx >= 0){
        int age = gameFrame - ordFrame[idx];
        if(age < 3){ ordSuppressed++; return false; }                     // 同一单位 3 帧内只收一条
        if(key != 0 && ordKey[idx] == key && age < ordGapOf(sn)){
            ordSuppressed++; return false;                                // 同一条令、冷却中
        }
    }
    if(ordersThisFrame >= 32){ ordSuppressed++; return false; }           // 每帧总量上限
    if(idx < 0){
        if(ordN >= CAP_SET) return true;                                  // 表满：放行，不记账
        idx = ordN++;
        ordSN[idx] = sn;
    }
    {   // 诊断：按类别计数
        int srt = sn / 10000;
        if(srt == SORT_FARMER)      ordFarmer++;
        else if(srt == SORT_ARMY)   ordArmy++;
        else                        ordBuild++;
    }
    ordFrame[idx] = gameFrame;
    ordKey[idx]   = key;
    ordersThisFrame++;
    return true;
}
int UsrAI::HumanAction(int SN, int obSN){
    if(!ordAllow(SN, ordKeyOf(INS_HUMANACTION, obSN, 0, 0, 0, 0.0, 0.0))) return -1;
    return AI::HumanAction(SN, obSN);
}
int UsrAI::HumanMove(int SN, double DR0, double UR0){
    if(!ordAllow(SN, ordKeyOf(INS_HUMANMOVE, 0, 0, 0, 0, DR0, UR0))) return -1;
    return AI::HumanMove(SN, DR0, UR0);
}
int UsrAI::HumanBuild(int SN, int BuildingNum, int blockDR, int blockUR){
    if(!ordAllow(SN, ordKeyOf(INS_HUMANBUILD, 0, BuildingNum, blockDR, blockUR, 0.0, 0.0))) return -1;
    return AI::HumanBuild(SN, BuildingNum, blockDR, blockUR);
}
int UsrAI::BuildingAction(int SN, int Action){
    if(!ordAllow(SN, ordKeyOf(INS_BUILDINGACTION, 0, Action, 0, 0, 0.0, 0.0))) return -1;
    return AI::BuildingAction(SN, Action);
}
int UsrAI::OrderCancel(int sn){
    if(!ordAllow(sn, ordKeyOf(INS_CANCEL, 0, 0, 0, 0, 0.0, 0.0))) return -1;
    return AddToIns(instruction(INS_CANCEL, sn, 0));
}
void UsrAI::processData()
{   
//#################################################################################################
    tagInfo info = getInfo();  //本帧快照
    getPhase(info);
    gameFrame = info.GameFrame;                      // 指令分流用：单调帧号
    ordTotal += ordersThisFrame;                     // 统计上一帧实际下了多少条
    ordersThisFrame = 0;
    /*if(info.GameFrame - lastOrdReportFrame >= 500){  // 诊断输出，不需要可删
        lastOrdReportFrame = info.GameFrame;
        std::cerr << "[UsrAI] frame=" << (int)info.GameFrame
                  << " orders/frame=" << (ordTotal / 500.0)
                  << " [farmer=" << (ordFarmer / 500.0) << " army=" << (ordArmy / 500.0) << " bldg=" << (ordBuild / 500.0) << "]"
                  << " suppressed=" << ordSuppressed << std::endl;
        ordTotal = 0; ordSuppressed = 0; ordFarmer = 0; ordArmy = 0; ordBuild = 0;
    }*/
    frameCount++;
 
    //找我方市镇中心\箭塔和对方大概来向
    if(mycenterDR < 0 || myarrowDR < 0 || myfameDR < 0){
        for(const tagBuilding& b : info.buildings){
            if(b.Type == BUILDING_CENTER){
                mycenterDR = b.BlockDR;
                mycenterUR = b.BlockUR;
                enemyDR = (MAP_L - 1 - mycenterDR)>=50?85:15;  //敌人在对角坐标处
                enemyUR = (MAP_U - 1 - mycenterUR)>=50?85:15;

            }
            if(b.Type == BUILDING_ARROWTOWER){
                myarrowDR = b.BlockDR;
                myarrowUR = b.BlockUR;

            }
            if(b.Type == BUILDING_GRANARY){
                myfameDR = b.BlockDR;
                myfameUR = b.BlockUR;
   
            }

            if(mycenterDR >= 0 && myarrowDR >= 0 && myfameDR >= 0) break;
        }
    }

// 造村民#################################################
    manageProduction(info);
// 建造、采集整体分配####################################################
    manageFarmer(info);         // 分配任务
    executeFarmerTasks(info);   // 采集任务
    if(frameCount % 25 == 0){
        manageBuild(info);       // 建造
        buildDepotIfNeeded(info);
    }
// 探路####################################################
    manageScout(info);       // 探路
// 科技研发####################################################
    manageResearch(info);
// 造兵########################################################
    if(info.civilizationStage>=CIVILIZATION_BRONZEAGE){
        manageArmyKite(info);
    }
// 防御####################################################
    if(isArmyReady) manageAttackKite(info);
    if(phase>=PHASE_DEFEND1&&phase<PHASE_ATTACK){
        manageDefense(info);
    }

// 升级时代####################################################
if(info.civilizationStage < CIVILIZATION_BRONZEAGE && info.Meat >=810){
    for(const tagBuilding& b : info.buildings){
        if(b.Type == BUILDING_CENTER){
            if(b.Project == ACT_NULL){
                //researchIfPossible(info,BUILDING_MARKET,BUILDING_MARKET_FARM_UPGRADE,200,50,0,0);
                BuildingAction(b.SN,BUILDING_CENTER_UPGRADE);
            }else if(b.Project == BUILDING_CENTER_CREATEFARMER && b.ProjectPercent <50){
                OrderCancel(b.SN);
            }
            break;
        }
    }
}

//  每秒打印实现#############################################
    if(frameCount % 25 == 0){    //每25帧（1秒）打印一次
        frameCount=0;
    }
//#################################################################################################
    
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int UsrAI::getPhase(const tagInfo& info){
    if(info.GameFrame>=25000) phase=PHASE_ATTACK;
    else if(info.GameFrame>=13500) phase=PHASE_DEFEND23;
    else if(info.GameFrame>=6600) phase=PHASE_DEFEND1;
    else phase=PHASE_ECON;
    return phase;
}

//%%%%%%%% 资源系统 %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// 选缺人最多的任务
int UsrAI::chooseTask(int cur[6]){
    int best = TASK_FOOD, bestGap = -99999;
    for(int i = 0;i < 6;i++){   
        int gap = taskTarget[i] - cur[i];
        if(gap > bestGap){ bestGap = gap; best = i;}    
    }
    return best;    
}

// 任务列表维护： 清理死者，按照人口调整目标，分派任务，空闲重置任务
void UsrAI::manageFarmer(const tagInfo& info){
    // 1. 按照人口调整目标人数
    int pop = (int)info.farmers.size();
    int xx= taskTarget[TASK_GOLD] = 2;
    if(info.civilizationStage>=CIVILIZATION_BRONZEAGE){
        xx++;
        taskTarget[TASK_GOLD]++;
    }
    int homenum=0;
    int rangenum=0;
    int collagenum=0;
    int stablenum=0;
    int towernum=0;
    for(const tagBuilding& b: info.buildings){
        if(b.Type==BUILDING_HOME && b.Percent>=100) homenum++;
        else if(b.Type == BUILDING_RANGE && b.Percent>=100) rangenum++; 
        else if(b.Type == BUILDING_COLLAGE && b.Percent>=100) collagenum++;
        else if(b.Type == BUILDING_STABLE&&b.Percent>=100) stablenum++;
        else if(b.Type == BUILDING_ARROWTOWER) towernum++;
    }
    if(rangenum>=rangeTarget){
        homeTarget = 13;
    }
    if(homenum>=13&&rangenum>=rangeTarget) xx+=taskTarget[TASK_BUILD] = 0;
    else xx+=taskTarget[TASK_BUILD] = 2;
    
    if(towernum>=towerTarget) needTower=false;
    if(needTower&&(towernum*150+info.Stone<=600)) xx+=taskTarget[TASK_STONE] = 2;
    else xx+=taskTarget[TASK_STONE] = 0;
    if(info.Wood>=400) xx+=taskTarget[TASK_WOOD] = (pop-xx)*20/100;
    else if(info.civilizationStage>=CIVILIZATION_BRONZEAGE){
        xx+=taskTarget[TASK_WOOD] = (pop-xx)*40/100;
    }else{
        xx+=taskTarget[TASK_WOOD] = (pop-xx)*50/100;
    }
    
    taskTarget[TASK_FOOD] = (pop-xx);
        // 石头够了 → 石农转伐木
    if(taskTarget[TASK_STONE] == 0 && !taskList[TASK_STONE].empty()){
        int moved = (int)taskList[TASK_STONE].size();
        for(int sn : taskList[TASK_STONE]) taskList[TASK_WOOD].push_back(sn);
        taskList[TASK_STONE].clear();
    }
    if(taskTarget[TASK_BUILD]==0 && !taskList[TASK_BUILD].empty()){
        int moved = (int)taskList[TASK_BUILD].size();
        for(int sn : taskList[TASK_BUILD]) taskList[TASK_FARM].push_back(sn);
        taskList[TASK_BUILD].clear();
        taskTarget[TASK_FARM] += moved;      // 同步目标，避免 chooseTask 判成超员
    }
    if(rangenum>=3){
        taskTarget[TASK_FARM] = taskTarget[TASK_FOOD];
        taskTarget[TASK_FOOD] -= taskTarget[TASK_FARM];
        for(int sn: taskList[TASK_FOOD]){
            taskList[TASK_FARM].push_back(sn);
        }
        taskList[TASK_FOOD].clear();
    }
    
    // ===== 正在建造/修理建筑的农民（含地基）：不参与任何抽调 =====
    FixSet busyBuilders;
    for(const tagFarmer& f : info.farmers)
        if(f.WorkObjectSN >= 0 && f.WorkObjectSN / 10000 == SORT_BUILDING)
            busyBuilders.add(f.SN);

     // 农田核算
    int farmSlots = 0;
    FixSet farmSN;
    for(const tagBuilding& b:info.buildings){
      if(b.Type != BUILDING_FARM) continue;
      farmSlots++;
      farmSN.add(b.SN);
    }
    int farmModeTarget = taskTarget[TASK_FARM];      // 本帧由“原食物目标”传入；>0 表示已进入种田阶段
    if(farmModeTarget > 0){
        // 先把"其它任务分剩下的人"算出来，全部给农田：目标总和 = 人口
        int usedAll = 0;
        for(int t = 0; t < 6; t++) if(t != TASK_FARM) usedAll += taskTarget[t];
        int rest = pop - usedAll;
        if(rest < 0) rest = 0;

        // 农田上限：既保留"田数+4"的节奏，又能兜住所有富余的人
        int farmWant = std::min(farmModeTarget, std::max(farmSlots + 4, rest));
        taskTarget[TASK_FARM] = farmWant;
        // 超编回收：正在种田/建田的保留，其余放回采集（木头不够优先转伐木）
        int recycleTo = (info.Wood < BUILD_FARM_WOOD) ? TASK_WOOD : TASK_FOOD;
        for(size_t i = 0; i < taskList[TASK_FARM].size()
                         && (int)taskList[TASK_FARM].size() > farmWant; ){
            int sn = taskList[TASK_FARM][i];
            bool busyFarm = false;
            for(const tagFarmer& f : info.farmers){
                if(f.SN == sn){
                    busyFarm = (f.WorkObjectSN >= 0 && farmSN.has(f.WorkObjectSN) > 0);
                    break;
                }
            }
            if(busyFarm){ i++; continue; }
            taskList[TASK_FARM].erase(taskList[TASK_FARM].begin() + i);
            if(recycleTo >= 0) taskList[recycleTo].push_back(sn);
        }

        // 缺人补回：从超编的采集任务抽人回农田（田建起来后，伐木的人会回来）
        int farmNeed = farmWant - (int)taskList[TASK_FARM].size();
        while(farmNeed > 0){
            int pick = -1, pickSurplus = 0;
            for(int t = 0; t < 6; t++){
                if(t == TASK_FARM || t == TASK_BUILD) continue;
                int surplus = (int)taskList[t].size() - taskTarget[t];
                if(surplus > pickSurplus){ pickSurplus = surplus; pick = t; }
            }
            if(pick < 0) break;                      // 没有超编任务可抽
                        int takeIdx = -1;                                   // 从后往前找"没在建造"的
            for(int i = (int)taskList[pick].size()-1;i >= 0;i--){
                if(busyBuilders.has(taskList[pick][i])) continue;
                takeIdx = i; break;
            }
            if(takeIdx < 0) break;                              // 该任务里全在建造 → 本轮不补了
            taskList[TASK_FARM].push_back(taskList[pick][takeIdx]);
            taskList[pick].erase(taskList[pick].begin() + takeIdx);
            farmNeed--;
        }
    }

         // ===== 超编回收：采集类之间互通（食物/木/石/金），多出来的补最缺人的 =====
    bool noMove[6] = {false,false,false,false,false,false};
    for(int iter = 0; iter < 12; iter++){
        int over = -1, overGap = 0, under = -1, underGap = 0;
        for(int t = 0;t < 6;t++){
            if(t == TASK_BUILD|| noMove[t]) continue;
            int gap = (int)taskList[t].size() - taskTarget[t];
            if(gap > overGap){ overGap = gap; over = t; }
            if(gap < underGap){ underGap = gap; under = t; }
        }
        if(over < 0 || under < 0) break;

        int pick = -1;                                          // 从后往前找"没在建造"的人
        for(int i = (int)taskList[over].size()-1;i >= 0;i--){
            if(busyBuilders.has(taskList[over][i])) continue;  // 在建造 → 跳过
            pick = i; break;
        }
        if(pick < 0){ noMove[over] = true; continue; }            // 该任务里全在建造 → 本轮换别的任务抽
        taskList[under].push_back(taskList[over][pick]);
        taskList[over].erase(taskList[over].begin() + pick);
    }

    // 2. 清理死掉的村民
    FixSet alive;
    for(const tagFarmer& f : info.farmers) alive.add(f.SN);
    for(int t=0;t<6;t++){
        auto& v = taskList[t];
        for(size_t i=0;i<v.size();){
            if(!alive.has(v[i])) v.erase(v.begin() + i);
            else i++;
        }
    }
    // 3. 统计人数，分派任务
    int cur[6] = {0,0,0,0,0};
    FixSet assigned;
    for(int t = 0; t<6; t++){
        cur[t] = (int)taskList[t].size();
        for(int sn : taskList[t]) assigned.add(sn);
    }
    // 4. 新村民分配任务
    for(const tagFarmer& f : info.farmers){
        if(assigned.has(f.SN)) continue;
        int t = chooseTask(cur);
        taskList[t].push_back(f.SN);
        cur[t]++;
    }
}

// ===== 快照线性查找：替代原来的 std::map<SN, 指针>（零动态分配）=====
static const tagFarmer* farmBySN(const tagInfo& info, int sn){
    for(const tagFarmer& f : info.farmers) if(f.SN == sn) return &f;
    return nullptr;
}
static const tagResource* resBySN(const tagInfo& info, int sn){
    for(const tagResource& r : info.resources) if(r.SN == sn) return &r;
    return nullptr;
}

// 执行采集任务
void UsrAI::executeFarmerTasks(const tagInfo& info){    
    FixSet allFarmSN;                       // 自家农田（种田也算"食物活"）
    for(const tagBuilding& b : info.buildings)
        if(b.Type == BUILDING_FARM) allFarmSN.add(b.SN);

    // 统计每个资源上的村民。只在当前快照中线性查找，避免 AI 线程里的
    // std::map 分配和容器析构，也避免保存跨帧指针。
    FixCount onTarget;
    for(const tagFarmer& f:info.farmers)
        if(f.WorkObjectSN >= 0) onTarget.inc(f.WorkObjectSN);

    // 已占用资源
    FixSet takenSource;
    for(int i = 0; i < onTarget.size(); i++){
        int targetSN = onTarget.keyAt(i);
        const tagResource* resource = resBySN(info, targetSN);
        if(resource == nullptr) continue;
        int tp = resource->Type;
        if(tp == RESOURCE_BUSH){
            takenSource.add(targetSN);                                   // 浆果：已有人
        }else if((tp == RESOURCE_GAZELLE) && resource->Blood <= 0){
            takenSource.add(targetSN);                                   // 尸体：已有人
        }
    }

    // 尸体超员判断
    for(int sn: taskList[TASK_FOOD]){
        const tagFarmer* farmer = farmBySN(info, sn);
        if(farmer == nullptr) continue;
        const tagFarmer& f = *farmer;
        if(f.WorkObjectSN<0) continue;
        const tagResource* resource = resBySN(info, f.WorkObjectSN);
        bool onCorpse = resource != nullptr
                        && (resource->Type == RESOURCE_GAZELLE
                        || resource->Type == RESOURCE_ELEPHANT)
                        && resource->Blood <= 0;
        if(onCorpse && onTarget.get(f.WorkObjectSN) > 1){
            onTarget.dec(f.WorkObjectSN);
            int newT = findNearestAliveAnimal(info,f.DR,f.UR);
            if(newT<0) newT = findNearestFood(info,f.DR,f.UR,takenSource);
            if(newT>=0 && newT!= f.WorkObjectSN){
                HumanAction(f.SN,newT);
                takenSource.add(newT);
            }
        }  
    }
    // 空闲农民分派
    for(int t = 0;t<6;t++){
        if(t == TASK_BUILD) continue;
                if(t == TASK_FARM){
            // 一人一田：已占用的农田
            FixSet farmSN, takenFarm;
            bool farmBuilding = false;                 // 是否已有农田地基在造
            for(const tagBuilding& b : info.buildings){
                if(b.Type != BUILDING_FARM) continue;
                farmSN.add(b.SN);
                if(b.Percent < 100) farmBuilding = true;
            }
            for(const tagFarmer& f : info.farmers)
                if(f.WorkObjectSN >= 0 && farmSN.has(f.WorkObjectSN))
                    takenFarm.add(f.WorkObjectSN);

            bool builderOrdered = false;               // 本帧最多派 1 人去建新田
            for(int sn : taskList[t]){
                const tagFarmer* farmer = farmBySN(info, sn);
                if(farmer == nullptr) continue;
                const tagFarmer& f = *farmer;

                // 正在种某块田的人 → 不动
                if(f.WorkObjectSN >= 0 && farmSN.has(f.WorkObjectSN)) continue;
                if(f.WorkObjectSN >= 0 && f.WorkObjectSN / 10000 == SORT_BUILDING) continue;

                // ① 空闲的 → 去种没人种的现成农田
                    int freeFarm = findNearestFreeFarm(info, f.DR, f.UR, takenFarm);
                    if(freeFarm >= 0){
                        HumanAction(sn, freeFarm);
                        takenFarm.add(freeFarm);
                        continue;
                    }
                

                // 建新田：要求 IDLE（HumanBuild 会打断采集），每帧只派一个
                if(!builderOrdered && !farmBuilding
                   && (int)farmSN.size() < farmerTarget
                   && info.Wood >= BUILD_FARM_WOOD
                   && countBuilding(info,BUILDING_MARKET) > 0){
                    int bd = -1, bu = -1;
                    if(findBuildSite(info,BUILDING_FARM,myfameDR,myfameUR,bd,bu)){
                        HumanBuild(sn, BUILDING_FARM, bd, bu);   // 建完他自动接手这块田
                        builderOrdered = true;
                        farmBuilding = true;
                        continue;
                    }
                }

                // 兜底：空闲又没事干 → 采野食，别闲着
                if(f.NowState == HUMAN_STATE_IDLE){
                    int aSN = findNearestAliveAnimal(info, f.DR, f.UR);
                    int foodSN = findNearestFood(info, f.DR, f.UR, takenSource);
                    int food = (aSN >= 0) ? aSN : foodSN;
                    if(food >= 0){ HumanAction(sn, food); takenSource.add(food); }
                }
            }
            continue;
        }else if(t == TASK_GOLD){
            for(int sn:taskList[t]){
                const tagFarmer* farmer = farmBySN(info, sn);
                if(farmer == nullptr) continue;
                const tagFarmer& f = *farmer;
                if(f.WorkObjectSN >= 0 && f.WorkObjectSN / 10000 == SORT_BUILDING) continue;
                const tagResource* workingResource = resBySN(info, f.WorkObjectSN);
                if(workingResource != nullptr && workingResource->Type == RESOURCE_GOLD) continue;  // 已在采金 → 不打扰

                int goldSN = findNearestResource(info,RESOURCE_GOLD,f.DR,f.UR);
                if(goldSN<0) continue;
                if(resBySN(info, goldSN) == nullptr) continue;
                HumanAction(sn, goldSN);                   // 正常采金
            }
            continue;
        }
        for(int sn : taskList[t]){
            const tagFarmer* farmer = farmBySN(info, sn);
            if(farmer == nullptr) continue;
            const tagFarmer& f = *farmer;
            
            // 他正在干本任务该干的活吗？是 → 不打扰；否（空闲 或 干错活）→ 重新指派
            bool rightJob = false;
            if(f.WorkObjectSN >= 0 && f.WorkObjectSN / 10000 == SORT_BUILDING){
                rightJob = true;                 // ★ 手上是建筑（在建/在修）→ 当"在干对的事"，不打扰
            }else if(t == TASK_WOOD || t == TASK_STONE){
                int wantRes = (t == TASK_WOOD) ? RESOURCE_TREE : RESOURCE_STONE;
                const tagResource* workingResource = resBySN(info, f.WorkObjectSN);
                rightJob = (workingResource != nullptr && workingResource->Type == wantRes);
            }else{                                  // TASK_FOOD：浆果/羚羊/自家农田 都算对
                if(allFarmSN.has(f.WorkObjectSN)) rightJob = true;
                else{
                    const tagResource* workingResource = resBySN(info, f.WorkObjectSN);
                    rightJob = (workingResource != nullptr &&
                                (workingResource->Type == RESOURCE_BUSH ||
                                 workingResource->Type == RESOURCE_GAZELLE));
                }
            }
            if(rightJob) continue;                  // ★ 在干对的事就别动他（避免每帧刷指令）

            int rsn = -1;
            if(t== TASK_FOOD){
                // 浆果、羚羊
                int aSN =findNearestAliveAnimal(info,f.DR,f.UR);
                int fSN = findNearestFood(info,f.DR,f.UR,takenSource);
                if(aSN<0){
                    rsn = fSN;
                }else if(fSN<0){
                    rsn = aSN;
                }else{
                    const tagResource* animal = resBySN(info, aSN);
                    const tagResource* food = resBySN(info, fSN);
                    double da = animal != nullptr ? calDistance(f.DR,f.UR,animal->DR,animal->UR) : 1e18;
                    double df = food != nullptr ? calDistance(f.DR,f.UR,food->DR,food->UR) : 1e18;
                    rsn =(da <= df) ? aSN: fSN;
                }

            }else{
                int resType = (t == TASK_WOOD) ? RESOURCE_TREE :
                              (t == TASK_STONE) ? RESOURCE_STONE :
                                           RESOURCE_GOLD;
                rsn = findNearestResource(info,resType,f.DR,f.UR);
            }
            if(rsn>= 0){
                takenSource.add(rsn);
                HumanAction(f.SN,rsn);
            }
        }
    }
}

// 就近补仓库/谷仓：只要"有人在采 + 离同类仓库/市中心太远"就补，与农民是否空闲无关
void UsrAI::buildDepotIfNeeded(const tagInfo& info){
    if(info.GameFrame - lastDepotFrame < 25) return;
    lastDepotFrame = info.GameFrame;                 // ★ 真节流：1 秒最多进一次（不管有没有下单）
    
    // ===== 有建到一半的仓库/谷仓地基 → 先派人接续=====
    for(const tagBuilding& b : info.buildings){
        if(b.Type != BUILDING_GRANARY && b.Type != BUILDING_STOCK) continue;
        if(b.Percent >= 100) continue;                     // 已建成
        bool hasBuilder = false;                           // 已经有人在建它？
        for(const tagFarmer& f : info.farmers)
            if(f.WorkObjectSN == b.SN){ hasBuilder = true; break; }
        if(hasBuilder) continue;

        int bestSN = -1; double bestD = 1e18;              // 派最近的非建造中农民接着建
        for(const tagFarmer& f : info.farmers){
            if(f.WorkObjectSN >= 0 && f.WorkObjectSN / 10000 == SORT_BUILDING) continue;
            for(int bsn:taskList[TASK_BUILD]){
                if(bsn==f.SN) continue;
            }
            double d = calDistance(f.DR, f.UR, b.BlockDR*BLOCKSIDELENGTH, b.BlockUR*BLOCKSIDELENGTH);
            if(d < bestD){ bestD = d; bestSN = f.SN; }
        }
        if(bestSN >= 0){
            HumanAction(bestSN, b.SN);    // 继续建这个地基（同 manageBuild 的做法）
            return;
        }
    }
    for(const tagResource& r : info.resources){
        int want = -1, ref = -1;
        if(r.Type == RESOURCE_TREE || r.Type == RESOURCE_GOLD || r.Type == RESOURCE_STONE
           || (r.Type == RESOURCE_GAZELLE && r.Blood <= 0)){      // 木/金/石/猎物肉 → 仓库
            want = BUILDING_STOCK;    ref = BUILDING_STOCK;
        }else if(r.Type == RESOURCE_BUSH){                        // 浆果 → 谷仓
            want = BUILDING_GRANARY;  ref = BUILDING_GRANARY;
        }else continue;

        bool used = false;                                        // 有人在采这个点吗
        for(const tagFarmer& f : info.farmers)
            if(f.WorkObjectSN == r.SN){ used = true; break; }
        if(!used) continue;

        double dd = nearestDepotDist(info, r.BlockDR, r.BlockUR, ref, BUILDING_CENTER);
        if(dd <= depotRangeBlocks * BLOCKSIDELENGTH) continue;    // 够近 → 不补

        bool nearBuilding = false;                                // 附近已有同类在建 → 别重复
        for(const tagBuilding& b : info.buildings)
            if(b.Type == want && b.Percent < 100
               && calDistance(b.BlockDR,b.BlockUR,r.BlockDR,r.BlockUR) < 15){ nearBuilding = true; break; }
        if(nearBuilding) continue;

        int needWood = (want == BUILDING_GRANARY) ? BUILD_GRANARY_WOOD : BUILD_STOCK_WOOD;
        if(info.Wood < needWood) continue;

        int bd = -1, bu = -1;
        if(!findNearBuildSite(info, want, r.BlockDR, r.BlockUR, 10, bd, bu)) continue;  // 半径放大到10

        int bestSN = -1; double bestD = 1e18;                     // 最近的农民去建（会打断他手上的活）
        for(const tagFarmer& f : info.farmers){
            bool skikBuilder=false;
            for(int ff:taskList[TASK_BUILD]){
                if(f.SN==ff){
                    skikBuilder=true;
                    break;
                }
            }
            if(skikBuilder) continue;
            double d = calDistance(f.DR, f.UR, r.BlockDR*BLOCKSIDELENGTH, r.BlockUR*BLOCKSIDELENGTH);
            if(d < bestD){ bestD = d; bestSN = f.SN; }
        }
        if(bestSN < 0) continue;

        HumanBuild(bestSN, want, bd, bu);
        lastDepotFrame = info.GameFrame;
        return;                                                   // 一帧只补一个
    }
}


// 敌方锚点方圆 enemySafeBlocks 格内不活动（避免被侦查、引来敌人）
bool UsrAI::inEnemyZone(double dr, double ur){
    if(enemyDR < 0 || enemyUR < 0) return false;      // 还没定位敌人锚点 → 不限制
    double d = calDistance(dr, ur,
                           enemyDR * BLOCKSIDELENGTH,
                           enemyUR * BLOCKSIDELENGTH);
    return d <= enemySafeBlocks * BLOCKSIDELENGTH;
}

// 找最近的资源
int UsrAI::findNearestResource(const tagInfo& info,int resType,double dr,double ur)
{
    int bestSN = -1;
    double bestDis = 1e18;
    for(const tagResource& r : info.resources){
        if(r.Type !=resType) continue;
        if(inEnemyZone(r.DR,r.UR)) continue;
        double dist = calDistance(dr,ur,r.DR,r.UR);
        if(dist < bestDis){
            bestDis = dist;
            bestSN = r.SN;
        }
    }
    return bestSN;
}
// 找最近的活猎物（羚羊/象，Blood>0）；没有返回 -1
int UsrAI::findNearestAliveAnimal(const tagInfo& info, double dr, double ur){
    int bestSN = -1;
    double bestDis = 1e18;
    for(const tagResource& r : info.resources){
        if(r.Type != RESOURCE_GAZELLE) continue;
        if(r.Blood <= 0) continue;                 // 死了不算
        if(inEnemyZone(r.DR,r.UR)) continue;
        double dist = calDistance(dr,ur,r.DR,r.UR);
        if(dist < bestDis){ bestDis = dist; bestSN = r.SN; }
    }
    return bestSN;
}

// 找最近食物：尸体/浆果；已占用的尸体跳过（尸体一人一个，浆果可共享）
int UsrAI::findNearestFood(const tagInfo& info, double dr, double ur,
                            const FixSet& takenSource){
    int bestSN = -1;
    double bestDis = 1e18;
    for(const tagResource& r : info.resources){
        if(r.Type != RESOURCE_BUSH && r.Type != RESOURCE_GAZELLE) continue;
        bool isBush = (r.Type == RESOURCE_BUSH);
        bool isCorpse = (r.Type == RESOURCE_GAZELLE) && r.Blood <= 0;
        if(( isBush || isCorpse) && takenSource.has(r.SN)) continue;   // 尸体已被占
        if(inEnemyZone(r.DR,r.UR)) continue;
        double dist = calDistance(dr,ur,r.DR,r.UR);
        if(dist < bestDis){ bestDis = dist; bestSN = r.SN; }
    }
    return bestSN;
}
// 生产板块
void UsrAI::manageProduction(const tagInfo& info){
    if(info.Human_Num >= info.Human_MaxNum) return;
    if((int)info.farmers.size() >= farmerTarget) return;
    if(info.Meat < 50) return;
    for(const tagBuilding& b : info.buildings){
        if(b.Type != BUILDING_CENTER) continue;      // 只对市中心
        if(b.Project != ACT_NULL) continue;          // 判断市中心忙不忙
        BuildingAction(b.SN, BUILDING_CENTER_CREATEFARMER);
        return;
    }
}

// 找最近的、还没有人种的已建成农田（一人一田）
int UsrAI::findNearestFreeFarm(const tagInfo& info, double dr, double ur,
                               const FixSet& takenFarm){
    int bestSN = -1;
    double bestD = 1e18;
    for(const tagBuilding& b : info.buildings){
        if(b.Type != BUILDING_FARM) continue;
        if(b.Percent < 100) continue;              // 只算建成的
        if(takenFarm.has(b.SN)) continue;        // 已经有人 → 跳过
        double d = calDistance(dr, ur, b.BlockDR*BLOCKSIDELENGTH, b.BlockUR*BLOCKSIDELENGTH);
        if(d < bestD){ bestD = d; bestSN = b.SN; }
    }
    return bestSN;
}

double UsrAI::nearestDepotDist(const tagInfo& info,int dr,int ur,int type1,int type2){
    double best = 1e9;
    for(const tagBuilding& b: info.buildings){
        if(b.Type!=type1 && b.Type!=type2) continue;
        double d = calDistance(dr*BLOCKSIDELENGTH,ur*BLOCKSIDELENGTH,b.BlockDR*BLOCKSIDELENGTH,b.BlockUR*BLOCKSIDELENGTH);
        if(d<best) best = d;
    }
    return best;
}

bool UsrAI::findNearBuildSite(const tagInfo& info, int buildingType, int cdr, int cur,
                              int maxR, int& outDR, int& outUR){
    if(info.theMap == nullptr) return false;
    int size = buildBuildingSize(buildingType);
    for(int r = 2; r <= maxR; r++){
        for(int dx = -r; dx <= r; dx++){
            for(int dy = -r; dy <= r; dy++){
                if(abs(dx) != r && abs(dy) != r) continue;
                int dr = cdr + dx, ur = cur + dy;
                if(dr < 0 || ur < 0 || dr + size > MAP_L || ur + size > MAP_U) continue;
                if(dr == lastFailDR && ur == lastFailUR) continue;
                if(canBuildHere(info,dr,ur) && !overlapsBuilding(info,dr,ur,size)
                   && !unitOnSite(info,dr,ur,size)){
                    outDR = dr; outUR = ur;
                    return true;
                }
            }
        }
    }
    return false;
}

//%%%%%%%% 建筑系统 %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// 建筑板块
void UsrAI::manageBuild(const tagInfo& info){
    // 建筑失败处理
    if(lastBuildOrder >= 0 && info.ins_ret.count(lastBuildOrder)){
        int ret = info.ins_ret.at(lastBuildOrder);
        if(ret != ACTION_SUCCESS){
            if(ret == ACTION_INVALID_HUMANBUILD_OVERLAP
               || ret == ACTION_INVALID_HUMANBUILD_UNEXPLORE
               || ret == ACTION_INVALID_HUMANBUILD_OVERBORDER
               || ret == ACTION_INVALID_HUMANBUILD_DIFFERENTHIGH
               || ret == ACTION_INVALID_HUMANBUILD_LOCK){
                // 位置失败：拉黑该点，下次换位置（资源不足等非位置失败不拉黑）
                if(lastBuildDR >= 0){
                    lastFailDR = lastBuildDR;
                    lastFailUR = lastBuildUR;
                }
            }
        }else{
            lastFailDR = -9999;
            lastFailUR = -9999;
        }
        lastBuildOrder = -1;
    }
    // 建造优先级：房 -> 市民 -> 兵营 -> 箭塔
    int wantType = -1;
    if(countBuilding(info,BUILDING_HOME) < homeTarget) wantType = BUILDING_HOME;
    else if(countBuilding(info,BUILDING_MARKET) < marketTarget) wantType = BUILDING_MARKET;
    else if(countBuilding(info,BUILDING_ARMYCAMP) < campTarget) wantType = BUILDING_ARMYCAMP;
    else if(countBuilding(info, BUILDING_ARROWTOWER) < towerTarget){
        // 箭塔科技判断
        bool arrowTechReady = arrowTechOrdered;
        if(arrowTechReady){
            for(const tagBuilding& b : info.buildings){
                if(b.Type == BUILDING_GRANARY && b.Project != ACT_NULL){
                   arrowTechReady = false;     
                }
            }
            wantType = BUILDING_ARROWTOWER;
        }
    }else if(countBuilding(info,BUILDING_STABLE) < stableTarget) wantType = BUILDING_STABLE;
    else if(countBuilding(info,BUILDING_RANGE) < rangeTarget) wantType = BUILDING_RANGE;
    else if(countBuilding(info,BUILDING_COLLAGE)<collageTarget&&info.civilizationStage>=CIVILIZATION_BRONZEAGE) wantType = BUILDING_COLLAGE;
    

    if(wantType >=0){
        int buildDR = -1;
        int buildUR = -1;
        for(const tagBuilding& b: info.buildings){
            if(b.Type == wantType && b.Percent < 100){
                buildDR = b.BlockDR;
                buildUR = b.BlockUR;
                break;
            }
        }
        if(buildDR < 0){
            // 已有地基则继续建；没有地基新建
            
            if(wantType == BUILDING_ARROWTOWER){
                if(info.Stone<BUILD_ARROWTOWER_STONE) return;   // 石头不够，暂缓建造
                if(!findBuildSite(info,wantType,myarrowDR,myarrowUR,buildDR,buildUR)){
                    return;
                }
            }else{
                int wantWood = (wantType == BUILDING_HOME) ? BUILD_HOUSE_WOOD :
                           (wantType == BUILDING_MARKET) ? BUILD_MARKET_WOOD :
                           (wantType == BUILDING_ARMYCAMP)? BUILD_ARMYCAMP_WOOD:
                           BUILD_COLLAGE_WOOD;
                 if(info.Wood < wantWood) return;   // 木头不够，暂缓建造
                // 找不到可建造点：让空闲建造工回市中心待命，避免干站不动
                if(!findBuildSite(info,wantType,mycenterDR,mycenterUR,buildDR,buildUR)){
                    if(mycenterDR >= 0){
                      for(int sn : taskList[TASK_BUILD]){
                        for(const tagFarmer& f : info.farmers){
                            if(f.SN != sn) continue;
                            if(f.NowState == HUMAN_STATE_IDLE){
                                double d = calDistance(f.DR,f.UR,
                                    mycenterDR*BLOCKSIDELENGTH,mycenterUR*BLOCKSIDELENGTH);
                                if(d > 2*BLOCKSIDELENGTH)
                                    HumanMove(sn,mycenterDR*BLOCKSIDELENGTH,mycenterUR*BLOCKSIDELENGTH);
                            }
                            break;
                        }
                      }
                    }
                    return;
                }
            }
        }
        int foundationSN = -1;
        for(const tagBuilding& b : info.buildings){
            if(b.Type == wantType && b.Percent < 100){
                foundationSN = b.SN;
                break;
            }
        }
        bool newBuildOrdered = false;   // 新建地基只让一个建造工下命令，避免重复下单
        for(int sn : taskList[TASK_BUILD]){
            bool idle = false;
            for(const tagFarmer& f : info.farmers){
                if(f.SN ==sn && f.NowState == HUMAN_STATE_IDLE){
                    idle = true;
                    break;
                }
            }
            if(!idle) continue;
            if(canBuildHere(info,buildDR,buildUR)){
                if(foundationSN >= 0){
                    lastBuildOrder = HumanAction(sn,foundationSN);
                }else if(!newBuildOrdered){
                    lastBuildOrder = HumanBuild(sn,wantType,buildDR,buildUR);
                    lastBuildDR = buildDR;      // 记录本次新建位置，失败时用于拉黑
                    lastBuildUR = buildUR;
                    newBuildOrdered = true;
                }
            }else{
                   // 探路点，去探路
                   lastBuildOrder = HumanMove(sn,buildDR*BLOCKSIDELENGTH,buildUR*BLOCKSIDELENGTH);
                   break;
             }
        }
    }
}

// 是否可以在该位置建造
bool UsrAI::canBuildHere(const tagInfo& info,int dr,int ur){
    if(info.theMap == nullptr) return false;
    int limDR = (int)info.theMap->size();
    int limUR = (limDR>0)?(int)(*info.theMap)[0].size():0;
    if(dr < 0 || ur < 0 || dr >= limDR || ur >= limUR) return false;
    const tagTerrain& t = (*info.theMap)[dr][ur];
    return t.type == MAPPATTERN_GRASS && t.height == MAPHEIGHT_FLAT;
}

// 统计已建造的建筑数量
int UsrAI::countBuilding(const tagInfo& info,int type){
    int cnt = 0;
    for(const tagBuilding& b : info.buildings){
        if(b.Type == type && b.Percent >= 100) cnt++;
    }
    return cnt;
}

// 根据建筑类型返回建筑大小
int UsrAI::buildBuildingSize(int type){
    if(type == BUILDING_HOME || type == BUILDING_ARROWTOWER) return 2;
    return 3;
}
 
// 判断 (dr,ur) 大小 size 的地方是否有建筑重叠
bool UsrAI::overlapsBuilding(const tagInfo& info, int dr, int ur, int size){
    int candL = dr - 1, candR = dr + size;      // 候选 +1圈：[dr-1, dr+size)
    int candD = ur - 1, candU = ur + size;
    for(const tagBuilding& b : info.buildings){
        int bs = buildBuildingSize(b.Type);
        int bL = b.BlockDR - 1, bR = b.BlockDR + bs;   // 已有建筑 +1圈
        int bD = b.BlockUR - 1, bU = b.BlockUR + bs;
        if((candL < bR && candR > bL && candD < bU && candU > bD) || candR > MAP_L || candU > MAP_U) return true;
    }
    for(const tagResource& r : info.resources){
        int rL = r.BlockDR - 1, rR = r.BlockDR + 1;
        int rD = r.BlockUR - 1, rU = r.BlockUR + 1;
        if((candL < rR && candR > rL && candD < rU && candU > rD) || rR > MAP_L || rU > MAP_U) return true;
    }
    return false;
}

// 环形扫描寻址， 内圈-> 外圈
bool UsrAI::findBuildSite(const tagInfo& info, int buildingType,int myDR,int myUR, int& outDR, int& outUR){
    int size = buildBuildingSize(buildingType);
    int probeDR = -1,probeUR = -1;
    double probeD = 1e18;
    const int ddx[4]= {-1,1,0,0};
    const int ddy[4]= {0,0,-1,1};
    for(int r =2;r<= 30;r++){
        for(int dx = -r;dx<=r;dx++){
            for(int dy = -r;dy<=r;dy++){
                if(abs(dx)!=r && abs(dy)!= r) continue;
                int dr = myDR + dx;
                int ur = myUR + dy;
                if(dr<0 || ur<0 || dr + size>MAP_L || ur + size>MAP_U) continue;
                if(dr == lastFailDR && ur == lastFailUR) continue;
                if(canBuildHere(info,dr,ur) && !overlapsBuilding(info,dr,ur,size) && !unitOnSite(info,dr,ur,size)){
                    outDR = dr;
                    outUR = ur;
                    return true;
                }
                // 未探索点,记录相邻未探索点，日后探索相邻未探索点
                if(info.theMap!=nullptr&&(*info.theMap)[dr][ur].type == 0){
                    for(int k = 0;k<4;k++){
                        int adr = dr + ddx[k];
                        int aur = ur + ddy[k];
                        if(adr<0 || aur<0 || adr>=MAP_L || aur>=MAP_U) continue;
                        if(!canBuildHere(info,adr,aur)) continue;
                        double d = (double)((adr-mycenterDR)*(adr-mycenterDR) + (aur-mycenterUR)*(aur-mycenterUR));
                        if(d<probeD){
                            probeD = d;
                            probeDR = adr;
                            probeUR = aur;
                        }
                        break;
                    }
                }
            }
        }
    }
    if(probeDR >= 0){     // 没有可建造点，找到最近的未探索点
        for(int sn:taskList[TASK_BUILD]){
            HumanMove(sn,probeDR*BLOCKSIDELENGTH,probeUR*BLOCKSIDELENGTH);
        }
    }
    return false;        
}
 
// 判断是否有单位在建造范围内
bool UsrAI::unitOnSite(const tagInfo& info, int dr, int ur, int size){
    for(const tagFarmer& f : info.farmers){
        int bdr = (int)(f.DR / BLOCKSIDELENGTH);
        int bur = (int)(f.UR / BLOCKSIDELENGTH);
        if(bdr >= dr && bdr < dr + size && bur >= ur && bur < ur + size) return true;
    }
    for(const tagArmy& a : info.armies){
        int bdr = (int)(a.DR / BLOCKSIDELENGTH);
        int bur = (int)(a.UR / BLOCKSIDELENGTH);
        if(bdr >= dr && bdr < dr + size && bur >= ur && bur < ur + size) return true;
    }
    for(const tagArmy& a : info.enemy_armies){
        int bdr = (int)(a.DR / BLOCKSIDELENGTH);
        int bur = (int)(a.UR / BLOCKSIDELENGTH);
        if(bdr >= dr && bdr < dr + size && bur >= ur && bur < ur + size) return true;
    }
    return false;
}


// %%%%%%%%% 科技系统 %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// 科技板块
void UsrAI::manageResearch(const tagInfo& info){
    // ① 观察：任何"正在研发中"的动作都记下来（Project 就是当前动作）→ 之后不再重复下令
    for(const tagBuilding& b : info.buildings)
        if(b.Project != ACT_NULL) techDone.add(b.Project);
    if(techDone.has(BUILDING_GRANARY_ARROWTOWER)) arrowTechOrdered = true;   // 真开始研发了才算数

    // ② 每 25 帧才尝试一次，避免研发失败时每帧刷指令
    if(frameCount % 25 != 0) return;

    // ③ 经济科技
    researchIfPossible(info,BUILDING_MARKET ,BUILDING_MARKET_WOOD_UPGRADE   ,120, 75,0,0);  // 伐木：采集+50%
    researchIfPossible(info,BUILDING_GRANARY,BUILDING_GRANARY_ARROWTOWER   ,120,  0,50,0);  // 箭塔科技
    researchIfPossible(info,BUILDING_STOCK  ,BUILDING_STOCK_UPGRADE_USETOOL,100,  0,0,0);   // 工具使用

    // ④ 复合弓科技：资源够 且 已进入造兵阶段（青铜时代 + 靶场已建好）
    bool armyStage = (info.civilizationStage >= CIVILIZATION_BRONZEAGE);
    if(armyStage)
        researchIfPossible(info,BUILDING_RANGE,BUILDING_RANGE_UPGRADE_COMPOSITE_BOW,180,100,0,0);
        // ===== 复合弓兵增益：射程 +1/+1、弓兵护甲 +2/+2 =====
    if(info.GameFrame%100==0&&techDone.has(BUILDING_RANGE_UPGRADE_COMPOSITE_BOW) > 0){
        researchIfPossible(info,BUILDING_MARKET,BUILDING_MARKET_WOOD_UPGRADE ,120, 75,0,0);  // 射程+1
        researchIfPossible(info,BUILDING_MARKET,BUILDING_MARKET_CRAFT_UPGRADE,170,150,0,0); // 射程再+1（需伐木加工）
        researchIfPossible(info,BUILDING_STOCK ,BUILDING_STOCK_UPGRADE_DEFENSE_ARCHER,100,0,0,0); // 护甲+2
    }
}

// 尝试研发一个科技 == 没研发过 + 建筑存在且空闲 + 资源够
bool UsrAI::researchIfPossible(const tagInfo& info,int buildingType,int action,
         int needFood,int needWood,int needStone,int needGold){
            if(techDone.has(action)) return false;          // 已研发/在研发 → 不重复下令
            if(info.Meat < needFood || info.Wood < needWood || info.Stone < needStone
               || info.Gold < needGold) return false;
            for(const tagBuilding& b : info.buildings){
                if(b.Type == buildingType && b.Percent >=100 && b.Project == ACT_NULL){
                    BuildingAction(b.SN,action);
                    return true;
                }
            }
         return false;         
}

// %%%%%%%%% 探路系统 %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// 探路板块
void UsrAI::manageScout(const tagInfo& info)
{   
    if(phase!= PHASE_ECON) return;
    if(scoutState == SCOUT_DONE) return;
    if(scoutState == SCOUT_WAIT && phase != PHASE_ECON) return;
    const double B = BLOCKSIDELENGTH;

    // 找祭司
    if(priestSN < 0){
        for(const tagArmy& a : info.armies)
            if(a.Sort == AT_PRIEST){ priestSN = a.SN; break; }
        if(priestSN < 0) return;
    }
    const tagArmy* priest = nullptr;
    for(const tagArmy& a : info.armies)
        if(a.SN == priestSN){ priest = &a; break; }
    if(priest == nullptr) return;

    // 回家点（锚点兜底）
    double hx = (myarrowDR >= 0 ? myarrowDR : mycenterDR) * B;
    double hy = (myarrowUR >= 0 ? myarrowUR : mycenterUR) * B;

    // ===== WAIT =====
    if(scoutState == SCOUT_WAIT){
        if(mycenterDR >= 0 && info.GameFrame >= 100){
            scoutState = SCOUT_OUT;
        }
        return;
    }

    // ===== RETURN =====
    if(scoutState == SCOUT_RETURN){
        double d = calDistance(priest->DR, priest->UR, hx, hy);
        if(d < 2*B){
            if(info.GameFrame >= 5800 || phase != PHASE_ECON){
                scoutState = SCOUT_DONE;                       // 真收工
            }else{
                homeRestUntil = info.GameFrame + 25;          // 在家歇~10秒
                scoutState = SCOUT_REST;
                scoutTgtDR = -1; scoutTgtUR = -1;
            }
        }else if(scoutTgtDR != hx || scoutTgtUR != hy|| (priest->NowState != HUMAN_STATE_WALKING && info.GameFrame - lastOrderFrame >= 25)){
            scoutTgtDR = hx; scoutTgtUR = hy;
            lastOrderFrame = info.GameFrame;
            HumanMove(priestSN, hx, hy);
        }
        return;
    }

    // ===== REST：歇完重开一轮巡点 =====
    if(scoutState == SCOUT_REST){
        if(info.GameFrame < homeRestUntil) return;
        if(!netReady) buildScoutNet();
        netCur = -1;
        scoutTgtDR = -1; scoutTgtUR = -1;
        scoutState = SCOUT_OUT;
        return;
    }

    // ===== 收工条件：时间到 / 血量低于60% =====
    if(info.GameFrame >= 5800 || priest->Blood < priest->MaxBlood*6/10){
        scoutState = SCOUT_RETURN; scoutTgtDR = -1; scoutTgtUR = -1;
        return;
    }
    
    // ===== 路点网：每个点只处理一次；到达 / 走不到 / 有敌人 → 一律永久拉黑 =====
    if(!netReady) buildScoutNet();
    if(netL.empty()) return;

    // 每秒扫一次：看到的敌人周围 25 格内的网点全部永久拉黑
    if(info.GameFrame - lastScanFrame >= 25){
        lastScanFrame = info.GameFrame;
        for(const tagArmy& e : info.enemy_armies){
            if(e.Blood <= 0) continue;
            double edr = e.DR / B, eur = e.UR / B;
            for(size_t i = 0; i < netL.size(); i++)
                if(!netDone[i] && calDistance(netL[i], netU[i], edr, eur) < 25)
                    netDone[i] = 1;
        }
    }
    // 当前目标点中途被拉黑（敌人来了）→ 立刻放弃
    if(scoutTgtDR >= 0 && netCur >= 0 && netCur < (int)netL.size() && netDone[netCur]){
        scoutTgtDR = -1; scoutTgtUR = -1;
    }

    // ===== 没有当前目标 → 取下一个没处理过的点；没有则回家收工 =====
    if(scoutTgtDR < 0){
        if(netCur >= 0 && netCur < (int)netL.size()) netDone[netCur] = 1;   // 上一个点永久拉黑
        netCur = -1;
        for(int i = netNext; i < (int)netL.size(); i++){
            if(netDone[i]) continue;
            netCur = i; break;
        }
        if(netCur < 0){                                        // 全图都走过了 → 清空拉黑，再巡一遍
            for(size_t i = 0; i < netDone.size(); i++) netDone[i] = 0;
            netNext = 0; netCur = -1;
            scoutTgtDR = -1; scoutTgtUR = -1;
            return;
        }
        netNext    = netCur + 1;
        scoutTgtDR = netL[netCur]*B + B/2;
        scoutTgtUR = netU[netCur]*B + B/2;
        HumanMove(priestSN, scoutTgtDR, scoutTgtUR);
        lastOrderFrame = info.GameFrame;
        wpOrderFrame   = info.GameFrame;
        return;
    }

    // ===== 正在朝当前点走：到达 / 走不到 → 都算处理完（下一帧拉黑并取新点）=====
    double dW = calDistance(priest->DR, priest->UR, scoutTgtDR, scoutTgtUR);
    int needFrames = 125 + (int)(dW / B) * 13 * 2;
    bool stopped = fabs(priest->DR0 - priest->DR) < 0.5 && fabs(priest->UR0 - priest->UR) < 0.5;
    if(dW < 3*B || (stopped && dW < 5*B) || (info.GameFrame - wpOrderFrame >= needFrames)){
        scoutTgtDR = -1; scoutTgtUR = -1;
        return;
    }
    if(info.GameFrame - lastOrderFrame >= 25){                 // 兜底重下（点不变）
        HumanMove(priestSN, scoutTgtDR, scoutTgtUR);
        lastOrderFrame = info.GameFrame;
    }
}

// ===== 生成路点网：蛇形（一条龙）覆盖全图，从"我方角"出发、最后收回家里 =====
// 祭司视野 12 格，点距 15 格 → 每走完一条腿就把这条带子基本点亮
void UsrAI::buildScoutNet(){
    netL.clear(); netU.clear(); netDone.clear();
    netCur = -1; netNext = 0;
    if(mycenterDR < 0 || enemyDR < 0) return;

    const int inset = 10, step = 15, n = 6;                // 6×6 网格，只保留我方这半边
    const int sx = (mycenterDR < 50) ? 1 : -1;             // 从我方角那一侧开始
    const int sy = (mycenterUR < 50) ? 1 : -1;

    // 分界线（两垒连线的中垂线）： A*l + Bv*u = K
    //   "我方这边" = A*l + Bv*u - K <= 0（取等号就是正好在对角线上）
    const double A  = double(enemyDR - mycenterDR);
    const double Bv = double(enemyUR - mycenterUR);
    const double K  = ((double)enemyDR*enemyDR + (double)enemyUR*enemyUR
                       - (double)mycenterDR*mycenterDR - (double)mycenterUR*mycenterUR) / 2.0;
    const double DLEN = sqrt(A*A + Bv*Bv);        // |敌垒-本垒|
    const double LIM  = 10.0 * DLEN;              // 允许越过分界线 10 格
    for(int i = 0; i < n; i++){
        int l = (sx > 0) ? (inset + i*step) : (MAP_L - inset - i*step);
        for(int k = 0; k < n; k++){
            int row = (i % 2 == 0) ? k : (n - 1 - k);      // 蛇形：奇数列反向，避免横跳
            int u = (sy > 0) ? (inset + row*step) : (MAP_U - inset - row*step);
            if(A*l + Bv*u - K > LIM) continue;             // 敌方那半边超10格子 → 丢掉
            netL.push_back(l); netU.push_back(u);
        }
    }

    // 对角线上也要有路点：沿分界线每 ~9 格补一个整数格点
    //   （6×6 网格的点 l+u 只会是 95/110，永远不落在线上，所以必须单独补）
    if(fabs(Bv) > 1e-9){
        int lastL = -1000;
        for(int l = inset; l <= MAP_L - inset; l++){
            double uf = (K - A*l) / Bv;                    // 解出分界线上的 u
            int u = (int)(uf + (uf >= 0 ? 0.5 : -0.5));
            if(fabs(uf - double(u)) > 1e-6) continue;      // 不是整数格点 → 跳过
            if(u < inset || u > MAP_U - inset) continue;
            if(l - lastL < 9) continue;                    // 沿线间距
            lastL = l;
            netL.push_back(l); netU.push_back(u);
        }
    }
    netL.push_back(mycenterDR);                            // 最后回家，把这个环接上
    netU.push_back(mycenterUR);

    netDone.assign(netL.size(), 0);
    netReady = true;
}




// %%%%%%%%%%%%% 防御、反攻 板块 ####################################################
void UsrAI::manageDefense(const tagInfo& info)
{   

    // 找祭司
    const tagArmy* priest = nullptr;
    double hx, hy;
    for(const tagArmy& a : info.armies){
        if(a.Sort == AT_PRIEST){
            priestSN = a.SN;
            priest = &a;
            hx=a.DR;
            hy=a.UR;
            break;
        }
    }
    if(priestSN < 0) return;
    if(info.enemy_armies.empty()) return;
    else{
        
        bool threat = false;
        for(const tagArmy& e : info.enemy_armies){
           if(e.Blood <= 0) continue;
           if(calDistance(e.DR, e.UR, hx, hy) < 35 * BLOCKSIDELENGTH){ threat = true; break; }
        }
        if(!threat) return;
    }
    
    if(priest == nullptr){
        for(const tagArmy& a : info.armies){
            if(a.Sort == AT_PRIEST){
                priest = &a;
                break;
            }
        }
        if(priest == nullptr) return;
    }
    
    bool wehaveThower=false;
    
    // 敌人进入视野
    if(info.enemy_armies.empty()) return;
    
    // 找箭塔
    bool haveArrowTower = false;
    for(const tagBuilding& b : info.buildings){
        if(b.Type == BUILDING_ARROWTOWER && b.Percent>=100){
            haveArrowTower = true;
            int towerSN = b.SN;
            int attackTgt = -1;
            bool isPriestAttack = false;
            for(const tagArmy& e: info.enemy_armies){
                attackTgt = e.SN;
                if(e.WorkObjectSN == priestSN){
                   attackTgt = e.SN;
                   isPriestAttack = true;
                   break;
                }
            }
            if(!isPriestAttack){
                for(const tagArmy& e: info.enemy_armies){
                   attackTgt = e.SN;
                   if(e.WorkObjectSN == -1){
                        attackTgt = e.SN;
                        break;
                    }
                }
            }
            if(frameCount % 5==0 && attackTgt>=0){
                HumanAction(towerSN,attackTgt);
                break;
            }
        }
    }
    
    

    bool wehaveStoneThower=false;
    for(const tagArmy& a:info.armies){
       if(a.Sort==AT_STONE_THROWER){
        wehaveStoneThower=true;
        break;
        }
    
    }

    // 祭司转化
    // 转化成功与否 
    if(convertTarget >= 0){
        bool ours = false;
        for(const tagArmy& a: info.armies){
            if(a.SN == convertTarget){
                ours = true;
                convertTarget = -1;
                break;
            }
        }
        if(!ours){
            if(priest->WorkObjectSN != convertTarget){
                convertTarget = -1;
            }
        }
    }
    if(convertTarget < 0 && priest->ConvertCooldown <= 0){
        int bestBlood = -1;
        for(const tagArmy& e: info.enemy_armies){
            if(e.Blood > bestBlood ){
                if(info.GameFrame>=18000&&!wehaveStoneThower&&e.Sort!=AT_STONE_THROWER) continue;
                bestBlood = e.Blood; convertTarget = e.SN;
            }
            if(e.Sort == AT_STONE_THROWER){
                convertTarget = e.SN;
                break;
            }
        }
        if(convertTarget >= 0){
            HumanAction(priestSN,convertTarget);    
            return;
        }
    }
    // 我方士兵攻击 

    // 集火投石车

        // ===== 我方军队分配目标：投石车优先，其次最近敌人 =====
    if(haveStoneThrower >= 0){                   // 存活性用 haveStoneThrower 自己判断
        bool alive = false;
        for(const tagArmy& e : info.enemy_armies)
            if(e.SN == haveStoneThrower){ alive = true; break; }
        if(!alive) haveStoneThrower = -1;
    }
    for(const tagArmy& a : info.armies){
        if(a.SN == priestSN) continue;            // 祭司另管
        if(a.Sort == AT_STONE_THROWER){           // 投石车拉扯
            int    eSN = -1;
            double eD = 1e18, eDR = 0, eUR = 0;
            for(const tagArmy& e : info.enemy_armies){
                if(e.Blood <= 0) continue;                       // 死的不算
                double dd = calDistance(a.DR, a.UR, e.DR, e.UR);
                if(dd < eD){ eD = dd; eSN = e.SN; eDR = e.DR; eUR = e.UR; }
            }
            const double B    = BLOCKSIDELENGTH;
            const double KEEP = double(DIS_STONE_THROWER) - 1.0;  // 想保持 9 格（射程 10）
            const double PULL = KEEP - 3.0;                       // 敌人进到 6 格内 → 开始往后拉
            if(eSN >= 0 && eD <= PULL*B){
                double rx, ry;
                if(eD <= 3.0*B){                                  // ① 被贴死 → 撤回自家角
                    rx = (MAP_L - enemyDR)*B + B/2;
                    ry = (MAP_U - enemyUR)*B + B/2;
                }else{                                            // ② 拉扯：退到离它 9 格
                    double dx = a.DR - eDR, dy = a.UR - eUR;      // 从敌人指向自己
                    double len = sqrt(dx*dx + dy*dy);
                    if(len < 1e-6){ dx = 1; dy = 0; len = 1; }
                    rx = eDR + dx/len * KEEP * B;
                    ry = eUR + dy/len * KEEP * B;
                }
                if(rx < 2*B)         rx = 2*B;                    // 别退出地图
                if(ry < 2*B)         ry = 2*B;
                if(rx > (MAP_L-2)*B) rx = (MAP_L-2)*B;
                if(ry > (MAP_U-2)*B) ry = (MAP_U-2)*B;
                // 只有"目标点变了"才重发移动令，避免每帧刷指令打断攻击关系
                if(fabs(a.DR0 - rx) > 1.5*B || fabs(a.UR0 - ry) > 1.5*B)
                    HumanMove(a.SN, rx, ry);
                continue;                                         // 这一拍不派攻击目标
            }
        }
        bool curIsEnemy = false;                          // 手上目标还是"视野内活着的敌人"吗
        for(const tagArmy& e : info.enemy_armies)
            if(e.SN == a.WorkObjectSN){ curIsEnemy = true; break; }
        if(curIsEnemy) continue;                          // 正在打活敌人 
        int tgt = -1;
        for(const tagArmy& e : info.enemy_armies)  // ① 优先投石车
            if(e.Sort == AT_STONE_THROWER){ tgt = e.SN; break; }
        if(tgt < 0){                               // ② 否则打最近敌人
            int bestD = 1000000;
            for(const tagArmy& e : info.enemy_armies){
                int ddr = abs(a.DR - e.DR), dur = abs(a.UR - e.UR);
                int d = ddr*ddr + dur*dur;
                if(d < bestD){ bestD = d; tgt = e.SN; }
            }
        }
        if(tgt >= 0) HumanAction(a.SN, tgt);
    }
    // 没有箭塔，吸引敌人
    if(!haveArrowTower && frameCount%5==0){
        for(const tagArmy& a: info.armies){
            if(a.SN == priestSN){
                continue;
            }
            for(const tagArmy& e: info.enemy_armies){
                HumanAction(a.SN,e.SN);
            }
            break;
        }
    }
}

/*
// 造兵
void UsrAI::manageArmy(const tagInfo& info){
    // 统计兵力
    nHop=nCav=0;
    for(const tagArmy& a:info.armies){
        if(a.Sort == AT_HOPLITE || a.Sort == AT_STONE_THROWER) nHop++;
        else if(a.Sort == AT_CAVALRY)       nCav++;
        else if(info.GameFrame % 25 == 0){
            if(a.SN != priestSN){
                HumanAction(a.SN,a.SN);  // 杂兵自杀
            }
        }
    }
    if(nHop+nCav>=25) isArmyReady=true;
    
    int homenum=0;
    for(const tagBuilding &b: info.buildings){
        if(b.Type == BUILDING_HOME){
            homenum++;
        }
    }
    if(homenum*4<=nHop+nCav){
        return;
    }
    // 造方阵兵
    int iscreate=false;
    if(nHop < wantHoplite
       && info.Meat >= BUILDING_COLLAGE_CREATE_HOPLITE_FOOD
       && info.Gold >= BUILDING_COLLAGE_CREATE_HOPLITE_GOLD){
        for(const tagBuilding& b : info.buildings){
            if(b.Type != BUILDING_COLLAGE) continue;
            if(b.Percent < 100 || b.Project != ACT_NULL) continue;
            BuildingAction(b.SN, BUILDING_COLLAGE_CREATE_HOPLITE);
            iscreate=true;
            break;
        }
    }
    // 造骑兵
    if(!iscreate && nCav < wantCavalry
       && info.Meat >= BUILDING_STABLE_CREATE_CAVALRY_FOOD
       && info.Gold >= BUILDING_STABLE_CREATE_CAVALRY_GOLD){
        for(const tagBuilding& b : info.buildings){
            if(b.Type != BUILDING_STABLE) continue;
            if(b.Percent < 100 || b.Project != ACT_NULL) continue;
            BuildingAction(b.SN, BUILDING_STABLE_CREATE_CAVALRY);
            break;
        }
    }
}
    */
/*
void UsrAI::manageAttack(const tagInfo& info){
    const double B = BLOCKSIDELENGTH;

    // ===== 主目标锁：投石车优先；否则"离我军重心最近的敌人"。锁住不放，直到它死/消失 =====
    bool focusSeen = false;
    for(const tagArmy& e : info.enemy_armies)
        if(e.SN == focusThrowerSN){ focusSeen = true; break; }
    bool focusEngaged = false;
    for(const tagArmy& a : info.armies)
        if(a.SN != priestSN && a.WorkObjectSN == focusThrowerSN){ focusEngaged = true; break; }
    if(focusThrowerSN >= 0 && !focusSeen && !focusEngaged) focusThrowerSN = -1;   // 立刻解锁

    // ① 出现投石车 → 抢锁（投石车永远优先）
    bool lockedIsThrower = false;
    for(const tagArmy& e : info.enemy_armies)
        if(e.SN == focusThrowerSN && e.Sort == AT_STONE_THROWER){ lockedIsThrower = true; break; }
    if(!lockedIsThrower){
        for(const tagArmy& e : info.enemy_armies)
            if(e.Sort == AT_STONE_THROWER){ focusThrowerSN = e.SN; break; }
    }

    // ② 还没锁 → 选"离我军重心最近的敌人"当主目标
    if(focusThrowerSN < 0 && !info.enemy_armies.empty()){
        double cDR = 0, cUR = 0; int n = 0;
        for(const tagArmy& a : info.armies){
            if(a.SN == priestSN) continue;
            cDR += a.DR; cUR += a.UR; n++;
        }
        if(n > 0){
            cDR /= n; cUR /= n;
            double bestD = 1e18;
            for(const tagArmy& e : info.enemy_armies){
                double d = calDistance(e.DR, e.UR, cDR, cUR);
                if(d < bestD){ bestD = d; focusThrowerSN = e.SN; }
            }
        }
    }

    // ===== 1) 视野内有敌人：全军打主目标（真正集火）=====
    if(!info.enemy_armies.empty()){
        for(const tagArmy& a : info.armies){
            if(a.SN == priestSN) continue;                       // 祭司不出动
            if(focusThrowerSN >= 0 && a.WorkObjectSN != focusThrowerSN)
                HumanAction(a.SN, focusThrowerSN);               //  不 break，人人锁同一个
                break;
        }
        return;
    }
    
    // =================  视野内没敌人 → 朝敌方箭塔推进，到塔附近拆塔 =================
    int towerSN = -1, towerBL = -1, towerBU = -1; double bestTd = 1e18;
    int num=-1;
    int siegeSN=-1;
    for(const tagBuilding& b : info.enemy_buildings){
        if(b.Type==BUILDING_SIEGE){
            siegeSN=b.SN;
            continue;
        }
        num++;
        double d = calDistance(b.BlockDR, b.BlockUR, mycenterDR, mycenterUR);
        if(d < bestTd){ bestTd = d; towerSN = b.SN; towerBL = b.BlockDR; towerBU = b.BlockUR; }
    }
    for(const tagArmy& a : info.armies){
        if(a.SN == priestSN) continue;                         // 祭司不出动
        if(towerSN >= 0){
            double dT = calDistance(a.DR/B, a.UR/B, towerBL, towerBU);
            if(dT < 12){
                if(a.WorkObjectSN != towerSN) HumanAction(a.SN, towerSN);
            }else if(a.NowState == HUMAN_STATE_IDLE){
                HumanMove(a.SN, towerBL*B + B/2, towerBU*B + B/2);
                break;
            }
        }else{
            if(a.NowState == HUMAN_STATE_IDLE)
                HumanMove(a.SN, enemyDR*B + B/2, enemyUR*B + B/2);
                break;
        }
    }
    if(info.enemy_armies.empty()&&num!=-1&&num<=2&&siegeSN>=0){
        const tagArmy* pr = nullptr;
        for(const tagArmy& a:info.armies){
            if(a.SN==priestSN){
                pr = &a;
                break;
            }
        }
        if(pr->ConvertCooldown<=0&&pr->WorkObjectSN!=siegeSN) HumanAction(priestSN,siegeSN);
    }
}
*/
// %%%%%%%%%%%%% 风筝流反攻（复合弓兵）####################################################
// 复合弓兵射程 7 格；敌方守军受"离核心/攻城厂的活动半径"限制（选目标 20 格、追击 25 格、远程再扣射程）。
// 打法：始终把距离锁在 6~7 格——他进我退、他停我射，把守军耗死在活动边界上。
// 非复合弓兵、非投石车一律自杀；祭司完全不动（不参与进攻，也绝不能被删）。
void UsrAI::manageAttackKite(const tagInfo& info){
    //if(frameCount%5!=0) return;
    const double B = BLOCKSIDELENGTH;

    // 最近的敌方箭塔（攻城厂永不进攻击目标，留给祭司转化）
    int towerSN = -1, towerBL = -1, towerBU = -1; double bestTd = 1e18;
    for(const tagBuilding& b : info.enemy_buildings){
        if(b.Type != BUILDING_ARROWTOWER) continue;
        double d = calDistance(b.BlockDR, b.BlockUR, mycenterDR, mycenterUR);
        if(d < bestTd){ bestTd = d; towerSN = b.SN; towerBL = b.BlockDR; towerBU = b.BlockUR; }
    }
    // 攻城厂（终局给祭司转化用）
    int siegeSN = -1; bool siegeBuilt = false;
    int towernum=-1;
    for(const tagBuilding& b : info.enemy_buildings){
        if(b.Type == BUILDING_SIEGE){ siegeSN = b.SN; siegeBuilt = (b.Percent >= 100); }
        if(b.Type == BUILDING_ARROWTOWER) towernum+=(towernum==-1?2:1);
    }
    if(!(info.enemy_armies.empty()&&towernum==-1&&siegeSN>=0)){
   
       for(const tagArmy& a : info.armies){
          if(a.SN == priestSN) continue;
          if(a.Sort != AT_COMPOSITE_BOWMAN && a.Sort != AT_STONE_THROWER) continue;

          const bool isThrower = (a.Sort == AT_STONE_THROWER);
          const double shootR = isThrower ? double(DIS_STONE_THROWER)
                                        : double(DIS_COMPOSITE_BOWMAN);
          const double backR  = isThrower ? 8.0 : 5.0;
          const double wantD  = shootR - 1.0;
       
          int tgtSN = -1; double tgtD = 1e18, tdr = 0, tur = 0;
          for(const tagArmy& e : info.enemy_armies){
             double d = calDistance(a.DR, a.UR, e.DR, e.UR);
             if(d < tgtD){ tgtD = d; tgtSN = e.SN; tdr = e.DR; tur = e.UR; }
          }
        
        

          double mx = a.DR, my = a.UR;
          int mode;                                                   // 0=待命 1=推进 2=打人 3=后撤 4=拆塔

          if(tgtSN >= 0 && tgtD <= backR*B){
              mode = 3;                                               // ① 贴脸 → 往反向点跑（拉扯）
              mx = (MAP_L - enemyDR)*B + B/2;
              my = (MAP_U - enemyUR)*B + B/2;
          }else if(tgtSN >= 0 ){
             mode = 2;                                               // ② 射程内 → 打人
          }else if(towernum>=1){
              mode = 4;                                               // ③ 塔在射程内 → 拆塔
          }else{
              mode = 1;                                               // ④ 其余 → 直冲敌人锚点
              mx = enemyDR*B + B/2;
              my = enemyUR*B + B/2;
         }

          if(mx < 1*B) mx = 1*B;
          if(my < 1*B) my = 1*B;
          if(mx > (MAP_L-1)*B) mx = (MAP_L-1)*B;
          if(my > (MAP_U-1)*B) my = (MAP_U-1)*B;

         if(mode == 2){
             if(a.WorkObjectSN != tgtSN)   {HumanAction(a.SN, tgtSN);}      // 打人
         }else if(mode == 4){
             if(a.WorkObjectSN != towerSN) {HumanAction(a.SN, towerSN);}    // 拆塔
         }else if(mode == 1 || mode == 3){
             if(a.NowState == HUMAN_STATE_IDLE || info.GameFrame % 25 == 0)
                {HumanMove(a.SN, mx, my);}                                // 推进到敌方点 / 后撤到反向点
          }
        // mode == 0：原地待命
      }

    }
    
    // ===== 终局：敌人清空 + 箭塔拆完 + 攻城厂已建成 → 派祭司贴墙转化 =====
    if(towernum<=3&&siegeSN >= 0 && siegeBuilt && info.enemy_armies.empty()){
        for(const tagArmy& a : info.armies){
            if(a.SN != priestSN) continue;
            if(a.ConvertCooldown > 0) break;                 // 转化冷却没好就等
            if(a.WorkObjectSN != siegeSN)                    // 已在转化 → 不重发
                HumanAction(priestSN, siegeSN);              // 攻击关系自动把祭司带到墙边
            break;
        }
    }

}

// 只造复合弓兵 + 兵种清洗（非复合弓兵/非投石车一律自杀，祭司除外）
void UsrAI::manageArmyKite(const tagInfo& info){
    int nBow = 0, nThrow = 0;
    for(const tagArmy& a : info.armies){
        if(a.Sort == AT_COMPOSITE_BOWMAN)   nBow++;
        else if(a.Sort == AT_STONE_THROWER) nThrow++;
    }

    // 兵种清洗：留复合弓兵 / 投石车 / 祭司，其余自杀
    if(info.GameFrame>=25000&&info.GameFrame % 25 == 0){
        for(const tagArmy& a : info.armies){
            if(a.SN == priestSN) continue;
            if(a.Sort == AT_COMPOSITE_BOWMAN || a.Sort == AT_STONE_THROWER) continue;
            HumanAction(a.SN, a.SN);                                     // 自家单位互指 = 自毁
        }
    }

    if(!isArmyReady && nBow+nThrow >= wantBowman-10) isArmyReady = true;           // 弓兵成型 → 转入反攻

    // 科技：复合弓（180 食 + 100 木）
    if(nBow < wantBowman)
        researchIfPossible(info, BUILDING_RANGE, BUILDING_RANGE_UPGRADE_COMPOSITE_BOW, 180, 100, 0, 0);
    if(!techDone.has(BUILDING_RANGE_UPGRADE_COMPOSITE_BOW)) return;
    int homenum = countBuilding(info, BUILDING_HOME);                    // 房子不够先别造
    if(homenum*4 <= nBow + nThrow+24) return;

    // 造复合弓兵（40 食 + 20 金）
    if(info.GameFrame % 25 != 0) return;
    if(nBow+nThrow <30 
       && info.Meat >= BUILDING_RANGE_CREATE_COMPOSITE_BOWMAN_FOOD
       && info.Gold >= BUILDING_RANGE_CREATE_COMPOSITE_BOWMAN_GOLD){
        for(const tagBuilding& b : info.buildings){
            if(b.Type != BUILDING_RANGE) continue;
            if(b.Percent < 100 || b.Project != ACT_NULL) continue;
            BuildingAction(b.SN, BUILDING_RANGE_CREATE_COMPOSITE_BOWMAN);
            break;
        }
    }
}

// 动态射程：直接问引擎对象要"当前"攻击距离（含木材加工/工艺科技加成）
// getDis_attack() 返回像素（已经乘过 BLOCKSIDELENGTH），除回来就是格
void UsrAI::updateBowRange(const tagInfo& info){
    if(info.GameFrame - bowRangeFrame < 25) return;   // 25 帧采一次，减少跨线程读
    bowRangeFrame = info.GameFrame;
    // AI 线程只使用 tagInfo 快照。不能通过 g_Object 取主线程对象，
    // 否则对象死亡/删除时会产生悬空指针和数据竞争。
    // 复合弓兵的基础射程由策略常量提供，科技升级仍由原有研究流程处理。
    for(const tagArmy& a : info.armies)
        if(a.Sort == AT_COMPOSITE_BOWMAN) return;
}
