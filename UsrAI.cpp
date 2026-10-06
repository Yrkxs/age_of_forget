#include "UsrAI.h"
#include<set>
#include <iostream>
#include<unordered_map>
#include<list>
#include <cstdlib>

using namespace std;
tagGame tagUsrGame;
ins UsrIns;
/*##########DO NOT MODIFY THE CODE ABOVE##########*/

#include <cmath>

int gameFrame=-1;
static AI* g_ai = nullptr;                                            // 在 processData() 里赋值

static int    HumanAction   (int SN, int obSN)                       { return g_ai->HumanAction(SN, obSN); }
static int    HumanMove     (int SN, double dr, double ur)           { return g_ai->HumanMove(SN, dr, ur); }
static int    HumanBuild    (int SN, int type, int dr, int ur)       { return g_ai->HumanBuild(SN, type, dr, ur); }
static int    BuildingAction(int SN, int act)                        { return g_ai->BuildingAction(SN, act); }
static double calDistance   (double a, double b, double c, double d) { return g_ai->calDistance(a, b, c, d); }

    // manageInit()
    int lastframe15=1;
    int lastframe25=1;
    int lastframe50=1;
    int mycenterDR = -1, mycenterUR = -1;
    int enemyDR = -1,    enemyUR = -1;
    int myarrowDR = -1,  myarrowUR = -1;
    int myfameDR = -1,   myfameUR = -1;
    
    void manageInit(const tagInfo& info);

    // manageProduction();
    int farmerTarget = 20;

    void manageProduction(const tagInfo& info);

    // manageFarmer()
    map<int,int> humantask;
    map<int,int> humantaskType;
    map<int,int> farmerSN;
    map<int,int> foodSN;
    map<int,int> woodSN;
    map<int,int> stoneSN;
    map<int,int> goldSN;
    map<int,int> farmSN;
    map<int,int> freeSN;
    map<int,int> takenSource;
    int buildSN=-1;
    enum FarmerTask{T0,TASK_FOOD,TASK_WOOD,TASK_STONE,TASK_GOLD,TASK_FARM,TASK_BUILD};
    int taskTarget[7] = {0,0,0,0,0,0,0};
    // ===== 任务分组人数（在这里改）=====
    int wantFood  = 7;      // 采食（浆果+羚羊）
    int wantWood  = 7;      // 伐木
    int wantStone = 2;      // 采石
    int wantGold  = 3;      // 采金
    int wantBuild = 1;      // 建筑工（不建仓库/谷仓）
    bool farmTaken = false;  // 是否已经派人去种田（不设种田组；食物组里只要木头够就抽 1 个人去种）
    // ===== 临时建仓库/谷仓 =====
    map<int,int> depotJob;      // 农民SN -> 原任务
    map<int,int> depotJobPos;   // 农民SN -> 工地块(dr*1000+ur)
    int depotRange   = 20;           // 多少格内没有仓库/谷仓才去建
    int depotRichCnt = 150;          // 资源"挺多"的门槛
    double enemySafeBlocks = 30.0;

    void manageFarmer(const tagInfo& info); 
    int findNearestFood(const tagInfo& info,double dr,double ur,const std::map<int,int>& takenSource);
    int findNearestResource(const tagInfo& info,int resType,double dr,double ur,int selfSN=-1);
    bool findBuildSite(const tagInfo& info, int buildingType,int myDR,int myUR, int& outDR, int& outUR);
    int buildBuildingSize(int type);
    bool canBuildHere(const tagInfo &info, int dr, int ur,int size);
    bool overlapsBuilding(const tagInfo &info, int dr, int ur, int size);
    bool unitOnSite(const tagInfo &info, int dr, int ur, int size);
    bool inEnemyZone(double dr, double ur);
    int countBuilding(const tagInfo& info,int type);

    // manageBuild()
    int marketTarget = 1;
    int rangeTarget=1;
    int homeTarget=5;
    bool homeup=false;
    int campTarget = 1;
    int collageTarget=0;
    int stableTarget =0;
    int towerTarget = 3;
    bool arrowTechOrdered = false;
    bool needTower = true;
    double depotRangeBlocks = 20.0;

    void manageBuild(const tagInfo& info);

    // manageResearch()
    //std::set<int> techDone;

    void manageResearch(const tagInfo& info);
    bool researchIfPossible(const tagInfo &info, int buildingType, int action, int needFood, int needWood, int needStone, int needGold);

    // manageScout()
    double pointStartDR = -1, pointStartUR = -1;
    int    pointStartFrame = 0;  
    enum GamePhase{ PHASE_ECON, PHASE_DEFEND1, PHASE_DEFEND23, PHASE_ATTACK };
    enum ScoutState{ SCOUT_OUT, SCOUT_RETURN, SCOUT_REST, SCOUT_DONE };  // 加 REST
    int phase = PHASE_ECON;
    int scoutState=SCOUT_OUT;
    double lastPosDR = -1, lastPosUR = -1;   // ← 新增：上次采样位置
    int    stillCnt  = 0;                    // ← 新增：连续几次没动
    int priestSN = -1;
    double lastDR=-1.0,lastUR=-1.0;
    double scoutTgtDR = -1;
    double scoutTgtUR = -1;
    int lastOrderFrame = 0;
    int homeRestUntil = 0;
    double lastPDR = -1, lastPUR = -1;   // 上次采样时祭司所在位置（判断有没有真的在挪）
    int    stuckCnt = 0;                 // 连续多少次采样位置没变
    
    int netCur = -1;                 // 当前正在去的网点（-1=没有目标）
    int netNext = 0;                 // 下一个待处理网点的搜索起点（单调前进）
    int lastScanFrame = -1000;       // 上次扫敌人的帧
    bool netReady = false;           // 路点网是否已生成
    vector<double> netL,netU;
    vector<double> netDone;

    void manageScout(const tagInfo& info);
    int getPhase(int &gameFrame);
    void buildScoutNet(const tagInfo& info);

    // manageRally() 集结
    
    vector<int> rallyGroup;              // 正在集结的兵 SN（到齐后清空）
    set<int>    rallyDone;               // 已经放出去参战的兵 SN
    int         rallyBatch = 0;          // 已放出批次（0 = 第一批还没出）
    int         lastRallyOrder = -1000;  // 催集合的帧门
    static bool inList(const vector<int>& v, int sn){ for(size_t i=0;i<v.size();i++) if(v[i]==sn) return true; return false; }
    void manageRally(const tagInfo& info);
    // manageArmyKite()
    bool isArmyReady=false;
    int wantBowman = 30;
    void manageArmyKite(const tagInfo& info);
    // manageAttackKite()
    void manageAttackKite(const tagInfo& info);
    // manageDefinse()
    int towerSN=-1;
    int convertTarget = -1;

    void manageDefense(const tagInfo& info);
    // manageUpdate()
    void manageUpdate(const tagInfo& info);


// ===== 调试打印：这一拍干了什么 =====
static void DbgAct(const char* what, int buildSN, int act){
    
    std::cerr << "[do] f=" << gameFrame << "  " << what
              << "  建筑SN=" << buildSN << "  动作号=" << act << std::endl;
    
    
}
static void DbgBuild(const char* what, int farmerSN, int type, int bd, int bu){
    
    std::cerr << "[do] f=" << gameFrame << "  " << what
              << "  农民SN=" << farmerSN << "  建筑类型=" << type
              << "  位置=(" << bd << "," << bu << ")" << std::endl;
    
}
// ===== 每 500 帧打印一次我方现状 =====
static void DbgStatus(const tagInfo& info){
    /*
    if(gameFrame % 500 != 0) return;

    // 各类建筑数量（含在建地基），以及完工/在建总数
    int bt[BUILDING_TYPE_MAXNUM];
    for(int i = 0; i < BUILDING_TYPE_MAXNUM; i++) bt[i] = 0;
    int bDone = 0, bWip = 0;
    for(const tagBuilding& b : info.buildings){
        if(b.Type >= 0 && b.Type < BUILDING_TYPE_MAXNUM) bt[b.Type]++;
        if(b.Percent >= 100) bDone++; else bWip++;
    }

    // 军队构成
    int nBow = 0, nThrow = 0, nPriest = 0, nOther = 0;
    for(const tagArmy& a : info.armies){
        if(a.Sort == AT_COMPOSITE_BOWMAN)   nBow++;
        else if(a.Sort == AT_STONE_THROWER) nThrow++;
        else if(a.Sort == AT_PRIEST)        nPriest++;
        else                                nOther++;
    }

    // 任务分配（1食 2木 3石 4金 5田 6建筑工）+ 真空闲人数
    int tc[7] = {0,0,0,0,0,0,0};
    for(auto& kv : humantask){
        int t = kv.second;
        if(t >= 1 && t <= 6) tc[t]++;
    }
    int nIdle = 0;
    for(const tagFarmer& f : info.farmers)
        if(f.WorkObjectSN < 0 && f.NowState == HUMAN_STATE_IDLE) nIdle++;

    std::cerr << "[status] f=" << gameFrame
              << " civ=" << info.civilizationStage
              << " | food=" << info.Meat << " wood=" << info.Wood
              << " stone=" << info.Stone << " gold=" << info.Gold
              << " | pop=" << (int)info.Human_Num << "/" << (int)info.Human_MaxNum
              << " farmers=" << (int)info.farmers.size()
              << " idle=" << nIdle
              << " task(食/木/石/金/田/工)=" << tc[1] << "/" << tc[2] << "/" << tc[3]
              << "/" << tc[4] << "/" << tc[5] << "/" << tc[6]
              << " | army(弓/投/祭/他)=" << nBow << "/" << nThrow << "/" << nPriest << "/" << nOther
              << " | build(done/wip)=" << bDone << "/" << bWip
              << " center=" << bt[BUILDING_CENTER]
              << " home=" << bt[BUILDING_HOME]
              << " stock=" << bt[BUILDING_STOCK]
              << " granary=" << bt[BUILDING_GRANARY]
              << " market=" << bt[BUILDING_MARKET]
              << " farm=" << bt[BUILDING_FARM]
              << " tower=" << bt[BUILDING_ARROWTOWER]
              << " camp=" << bt[BUILDING_ARMYCAMP]
              << " stable=" << bt[BUILDING_STABLE]
              << " range=" << bt[BUILDING_RANGE]
              << " collage=" << bt[BUILDING_COLLAGE]
              << std::endl;
    */
}

void UsrAI::processData()
{   
//#################################################################################################
    g_ai = nullptr;  
    g_ai = this;
    tagInfo info = getInfo();  //本帧快照
    gameFrame = info.GameFrame;
    DbgStatus(info);  
    getPhase(gameFrame);

     //找我方市镇中心\箭塔和对方大概来向
    if(gameFrame-lastframe50>=50){
        manageInit(info);
    }
    
    // 造村民
    if(gameFrame-lastframe25>=25){
        manageProduction(info);
    }
    
    // 任务管理（采集+建造）
    if(gameFrame-lastframe25>=25){
       manageFarmer(info);
       manageBuild(info);
    }

    // 探路
    if(gameFrame-lastframe15>=15){
        manageScout(info);
    }

    // 科技研发
    if(gameFrame-lastframe50>=50){
        manageResearch(info);
    }

    // 造兵
    if(gameFrame-lastframe15>=15){
        manageArmyKite(info);
        //if(info.GameFrame>=25000) manageRally(info);
    }

    // 反攻
    if(isArmyReady&&gameFrame-lastframe15>=15) manageAttackKite(info);

    // 防御
    if(phase!=PHASE_ECON&&gameFrame-lastframe15>=15){
        manageDefense(info);
    }

    // 升级时代
    if(gameFrame-lastframe15>=15){
        manageUpdate(info);
        lastframe15=gameFrame;
    }
    if(gameFrame-lastframe25>=25){
        lastframe25=gameFrame;
    }
    if(gameFrame-lastframe50>=50){
        lastframe50=gameFrame;
    }
}

int getPhase(int& gameFrame){
    if(gameFrame >= 25000) phase = PHASE_ATTACK;
    else if(gameFrame>=13500) phase=PHASE_DEFEND23;
    else if(gameFrame>=5800) phase=PHASE_DEFEND1;
    else phase=PHASE_ECON;
    return phase;
}

void manageInit(const tagInfo& info)
{
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
}



void manageProduction(const tagInfo& info)
{
    if(info.Human_Num >= info.Human_MaxNum) return;
    if((int)info.farmers.size() >= farmerTarget) return;
    if(info.Meat < 50) return;
    for(const tagBuilding& b : info.buildings){
        if(b.Type != BUILDING_CENTER) continue;      // 只对市中心
        if(b.Project != ACT_NULL) continue;          // 判断市中心忙不忙
        BuildingAction(b.SN, BUILDING_CENTER_CREATEFARMER);
         DbgAct("生产村民", b.SN, BUILDING_CENTER_CREATEFARMER);
        return;
    }
}



void manageFarmer(const tagInfo& info)
{
    const double B = BLOCKSIDELENGTH;
    const double refDR = (mycenterDR >= 0) ? mycenterDR*B : 50*B;   //  找资源一律以市中心为基准
    const double refUR = (mycenterUR >= 0) ? mycenterUR*B : 50*B;
    takenSource.clear();
    farmTaken =false;

    // ---------- 0. 死掉的农民清出任务表（注意 erase 的写法）----------
    {
        std::map<int,int> alive;
        for(const tagFarmer& f : info.farmers) alive[f.SN] = 1;
        for(auto it = humantask.begin(); it != humantask.end(); ){
            if(!alive.count(it->first)){
                depotJob.erase(it->first);
                depotJobPos.erase(it->first);
                if(buildSN == it->first) buildSN = -1;
                humantaskType.erase(it->first);
                it = humantask.erase(it);
            } else ++it;
        }
        for(const tagFarmer& f : info.farmers)
            if(!humantask.count(f.SN)){ humantask[f.SN] = -1; humantaskType[f.SN] = -1; }
    }

    // ---------- 1. 目标人数：升级前田=0；升级后采食的人整体转种田 ----------
    int towernn=0;
    int r=0;
    int home=0;
    bool canBuildFarm = false;
    for(const tagBuilding& b:info.buildings){
        if(b.Type==BUILDING_ARROWTOWER && b.Percent>=100) towernn++;
        else if(b.Type==BUILDING_RANGE && b.Percent>=100) r++;
        else if(b.Type==BUILDING_HOME && b.Percent>=100) home++;
        else if(b.Type==BUILDING_MARKET && b.Percent>=100) canBuildFarm=true;
    }
    if(towernn>=3) needTower=false;
    if(r>=3){
        homeup=true;
    }
    const bool bronze = (r>=3);
    int tFarm  = 99;
    int tGold = wantGold;
    if(home == 13 && r>=3){
        wantFood +=wantBuild;
        wantBuild =0;
    }
    int tBuild = wantBuild;
    if(towernn*150+info.Stone>=450||!needTower){
            wantFood += wantStone;
            wantStone = 0;
    }
    int tStone = wantStone;
    int tWood  = wantWood;
    int tFood = wantFood;
    // ---------- 2. 资源独占：谁正在采哪块，先全部登记（同一块只会有一个人）----------
    for(const tagFarmer& f : info.farmers)
        if(f.WorkObjectSN >= 0) takenSource[f.WorkObjectSN] = 1;

    // ---------- 3. 人数平衡：只动空闲的人，正在干活的不打扰 ----------
    int cnt[8] = {0,0,0,0,0,0,0,0};
    for(auto& kv : humantask){
        int t = kv.second;
        if(t < 0 || t > 6){ t = -1; kv.second = -1; }
        if(t > 0) cnt[t]++;
    }
    int tgt[8] = {0, tFood, tWood, tStone, tGold, tFarm, tBuild, 0};

    // 3a. 超编 → 释放（临时建仓库的人不在 humantask 里，天然不受影响）
    for(int t = 1; t <= 6; t++){
        while(cnt[t] > tgt[t]){
            bool moved = false;
            for(auto& kv : humantask)
                if(kv.second == t){
                    kv.second = -1; humantaskType[kv.first] = -1; cnt[t]--;
                    if(t == TASK_BUILD) buildSN = -1;
                    if(t == TASK_STONE && tgt[TASK_STONE] == 0){ kv.second = TASK_WOOD; cnt[TASK_WOOD]++;}
                    moved = true; break;
                }
            if(!moved) break;
        }

    }
    // 3b. 缺编 → 用空闲的人补（顺序：金→石→木→食→田）
    static const int prio[6] = { TASK_BUILD, TASK_FOOD,TASK_STONE, TASK_WOOD, TASK_GOLD,TASK_FARM };
    for(int k = 0; k < 6; k++){
        int t = prio[k];
        if(t == TASK_FARM) continue;
        while(cnt[t] < tgt[t]){
            bool moved = false;
            for(auto& kv : humantask)
                if(kv.second == -1){
                    kv.second = t; humantaskType[kv.first] = -1; cnt[t]++;
                    if(t == TASK_BUILD) buildSN = kv.first;
                    moved = true; break;
                }
            if(!moved) break;
        }
    }
    // 建筑工：固定一个人（不参与上面的平衡；manageBuild 给他派活）
    if(tBuild > 0){
        if(buildSN < 0)
            for(auto& kv : humantask) if(kv.second == -1){ buildSN = kv.first; break; }
        if(buildSN >= 0 && humantask.count(buildSN)) humantask[buildSN] = TASK_BUILD;
    }

    // ---------- 4. 执行 ----------
    for(const tagFarmer& f : info.farmers){
        if(!humantask.count(f.SN)) continue;
        int t = humantask[f.SN];

        // 建筑工交给 manageBuild
        if(f.SN == buildSN) continue;

        // ---- 4.0 临时建仓库/谷仓：建完之前绝不被调走 ----
        auto dj = depotJob.find(f.SN);
        if(dj != depotJob.end()){
            int site = depotJobPos.count(f.SN) ? depotJobPos[f.SN] : -1;
            int bsn = -1;
            if(site >= 0)
                for(const tagBuilding& b : info.buildings)
                    if(b.BlockDR == site/1000 && b.BlockUR == site%1000 && b.Percent < 100){ bsn = b.SN; break; }
            if(bsn >= 0){
                if(f.NowState == HUMAN_STATE_IDLE && f.WorkObjectSN != bsn) HumanAction(f.SN, bsn);
                continue;                                     // ★ 建完前不调走、不派别的活
            }
            humantask[f.SN] = dj->second;                     // 建完了 → 回到原任务
            depotJob.erase(f.SN);
            depotJobPos.erase(f.SN);
            t = humantask[f.SN];
        }

        // ---- 4.1 手上已经在干"本任务该干的活" → 不打扰 ----
        bool handOk = false;
        if(f.WorkObjectSN >= 0){
            if(f.WorkObjectSN / 10000 == SORT_BUILDING) handOk = true;   // 在建/在修
            else for(const tagResource& r : info.resources)
                if(r.SN == f.WorkObjectSN){
                    handOk = (t == TASK_FOOD) ? (r.Type == RESOURCE_BUSH || r.Type == RESOURCE_GAZELLE)
                          : (t == TASK_WOOD) ? (r.Type == RESOURCE_TREE)
                          : (t == TASK_STONE)? (r.Type == RESOURCE_STONE)
                          : (t == TASK_GOLD) ? (r.Type == RESOURCE_GOLD) 
                          : (t == TASK_FARM) ? (r.Type == RESOURCE_TREE) : false;
                    break;
                }
        }

        if(t == TASK_FARM){
            if(f.WorkObjectSN >= 0){
                continue;
            }
            humantask[f.SN] = TASK_FOOD;
            humantaskType[f.SN] = -1;
            t = TASK_FOOD;
        }
        // ---- 4.2 种田：木头够（150）就强制去种，不管有没有建筑在建 ----

        if(canBuildFarm &&t == TASK_FOOD){
            static int farmTick = -1;
            static std::set<int> farmClaimed;                   // 本帧已被认领的田（含 -1 = 有人在建新田）
            if(farmTick != gameFrame){ farmTick = gameFrame; farmClaimed.clear(); }

            const int farmGate = 200;           // 150 = 建田 75 + 余量 75

            if(info.Wood >= farmGate){
                // ① 有空田 → 直接去种（每个轮到的空闲食物组农民都能领一块）
                int freeFarm = -1; double bestD = 1e18;
                for(const tagBuilding& b : info.buildings){
                    if(b.Type != BUILDING_FARM || b.Percent < 100) continue;
                    if(farmClaimed.count(b.SN)) continue;
                    bool used = false;
                    for(const tagFarmer& g : info.farmers)
                        if(g.WorkObjectSN == b.SN){ used = true; break; }
                    if(used) continue;
                    double dd = calDistance(f.DR, f.UR, b.BlockDR*B, b.BlockUR*B);
                    if(dd < bestD){ bestD = dd; freeFarm = b.SN; }
                }
                if(freeFarm >= 0){
                    farmClaimed.insert(freeFarm);
                    humantask[f.SN] = TASK_FARM;
                    HumanAction(f.SN, freeFarm);
                    continue;                                   // 只有真去种田的人 continue
                }
                // ② 没有空田 → 本帧只让一个人去建一块新田
                if(farmClaimed.empty()){
                    int bd = -1, bu = -1; bool built = false;
                    int tried[4]; int nTried = 0;
                    while(!built){
                        int gDR = -1, gUR = -1, gSN = -1; double bestD = 1e18;
                        for(const tagBuilding& b : info.buildings){
                            if(b.Type != BUILDING_GRANARY) continue;          // 只拿谷仓当地标
                            bool skip = false;
                            for(int k = 0; k < nTried; k++) if(tried[k] == b.SN){ skip = true; break; }
                            if(skip) continue;                                 // 这个已经试过了
                            double dd = calDistance(f.DR, f.UR, b.BlockDR*B, b.BlockUR*B);
                            if(dd < bestD){ bestD = dd; gDR = b.BlockDR; gUR = b.BlockUR; gSN = b.SN; }
                        }
                        if(gSN < 0) break;                                     // 谷仓都试完了
                        if(nTried < 4) tried[nTried++] = gSN;
                        if(findBuildSite(info, BUILDING_FARM, gDR, gUR, bd, bu)){
                            farmClaimed.insert(-1);
                            humantask[f.SN] = TASK_FARM;
                            HumanBuild(f.SN, BUILDING_FARM, bd, bu);
                            built = true;
                        }
                    }
                    if(built) continue;
                    // 两个谷仓旁边都建不下 → 退回市中心再试一次
                    if(findBuildSite(info, BUILDING_FARM, mycenterDR, mycenterUR, bd, bu)){
                        farmClaimed.insert(-1);
                        humantask[f.SN] = TASK_FARM;
                        HumanBuild(f.SN, BUILDING_FARM, bd, bu);
                        DbgBuild("建农田", f.SN, BUILDING_FARM, bd, bu);
                        continue;
                    }
                }
            }
        }
        if(handOk) continue;
        // ---- 4.3 采集类（食/木/石/金）：找一块"没人采"的资源 ----
        if(f.NowState != HUMAN_STATE_IDLE) continue;              // 在走过去/在干活 → 别插队
        int rt = (t == TASK_FOOD)  ? RESOURCE_BUSH  :
                 (t == TASK_WOOD)  ? RESOURCE_TREE  :
                 (t == TASK_STONE) ? RESOURCE_STONE : RESOURCE_GOLD;
        int rsn = findNearestResource(info, rt, f.DR, f.UR,f.SN);
        if(rsn < 0 && t == TASK_FOOD)
            rsn = findNearestResource(info, RESOURCE_GAZELLE, f.DR, f.UR);   // 食物：浆果没了就找羚羊
        if(rsn >= 0){
            takenSource[rsn] = 1;
            humantaskType[f.SN] = t;
            HumanAction(f.SN, rsn);
        }
    }
    // 建仓库 谷仓
    if(info.civilizationStage>= CIVILIZATION_BRONZEAGE) return;
    int gnum=0;
    int snum=0;
    for(const tagBuilding& b:info.buildings){
        if(b.Type==BUILDING_GRANARY) gnum++;
        else if(b.Type==BUILDING_STOCK) snum++;
    }
    int tWant = -1, wantTask = -1;
    double needDR = 0, needUR = 0;
    for(auto& kv : takenSource){
            int srt = kv.first / 10000;
            if(srt!=2&&srt!=5)continue;
            const tagResource* rp = nullptr;
            for(const tagResource& rr : info.resources)
                if(rr.SN == kv.first){ rp = &rr; break; }
            if(rp == nullptr) continue;
            const tagResource&r = *rp;
            rp = nullptr;
            int want = -1, task = -1, need = 0;
            if(r.Type == RESOURCE_BUSH)          { want = BUILDING_GRANARY; task = TASK_FOOD;  need = 5; }
            else if(r.Type == RESOURCE_GAZELLE)  { want = BUILDING_STOCK;   task = TASK_FOOD;  need = 4; }
            else if(r.Type == RESOURCE_TREE)     { want = BUILDING_STOCK;   task = TASK_WOOD;  need = 5; }
            else if(r.Type == RESOURCE_STONE)    { want = BUILDING_STOCK;   task = TASK_STONE; need = 5; }
            else if(r.Type == RESOURCE_GOLD)     { want = BUILDING_STOCK;   task = TASK_GOLD;  need = 1; }
            if(want < 0) continue;
            if(want==BUILDING_GRANARY && gnum>=3) continue;  // 已有谷仓 → 不建
            if(want==BUILDING_STOCK && snum>=3) continue;    // 已有仓库 → 不建

            int cost = (want == BUILDING_GRANARY) ? BUILD_GRANARY_WOOD : BUILD_STOCK_WOOD;
            if(info.Wood < cost) continue;                        // 木头不够 → 这一拍先不建

            bool hasNear = false;                                 // 20 格内已有对应仓库 → 不用建
            for(const tagBuilding& b : info.buildings){
                if(b.Type != want) continue;
                if(calDistance(b.BlockDR*B, b.BlockUR*B, r.DR, r.UR) <= depotRange*B){ hasNear = true; break; }
            }
            if(hasNear) continue;

            bool building = false;                                // 已有同类型地基在建 → 不重复
            for(const tagBuilding& b : info.buildings)
                if(b.Type == want && b.Percent < 100){ building = true; break; }
            if(building) continue;

            int nears = 0;                                         // ★ 周围同类型资源的块数
            for(const tagResource& o : info.resources){
                if(o.Type != r.Type) continue;
                if(calDistance(r.DR, r.UR, o.DR, o.UR) <= depotRange*B) nears++;
                if(nears >= need) break;
            }
            if(nears < need) continue;

            tWant = want; wantTask = task; needDR = r.DR; needUR = r.UR;
            break;                                                // 一拍最多派一个工地
    }
    if(tWant >= 0){
        int pickSN = -1;
        for(auto& kv : humantask)                              // ★ 强制：先抽"正在干这类活"的人
            if(kv.second == wantTask){ pickSN = kv.first; break; }
        if(pickSN < 0)
            for(auto& kv : humantask)                          // 该组没人（或都空闲）→ 抽个空闲的
               if(kv.second == -1){ pickSN = kv.first; break; }
        if(pickSN >= 0 && pickSN != buildSN){                  // 别占用建筑工
            int bd = -1, bu = -1;
            if(findBuildSite(info, tWant, (int)(needDR/B), (int)(needUR/B), bd, bu)){
                    depotJob[pickSN]    = humantask[pickSN];       // 记住原任务；建完自动回去
                    depotJobPos[pickSN] = bd*1000 + bu;
                    HumanBuild(pickSN, tWant, bd, bu);
                    DbgBuild("建仓库/谷仓", pickSN, tWant, bd, bu);
            }
        }
    }
    // ===== 临时诊断：谁在闲着（约每 20 秒打一次，定位完删） =====
    if(gameFrame % 500 == 0){
        for(const tagFarmer& f : info.farmers){
            if(f.WorkObjectSN >= 0) continue;
            if(f.NowState != HUMAN_STATE_IDLE) continue;
            std::cerr << "[idle] f=" << gameFrame
                      << " SN=" << f.SN
                      << " task=" << (humantask.count(f.SN) ? humantask[f.SN] : -99)
                      << " builder=" << (f.SN == buildSN ? 1 : 0)
                      << " depot=" << (depotJob.count(f.SN) ? 1 : 0)
                      << " pos=(" << f.DR/B << "," << f.UR/B << ")"
                      << std::endl;
        }
    }
}

int findNearestFood(const tagInfo& info,double dr,double ur,const std::map<int,int>& takenSource)
{
    int bestSN = -1;
    double bestDis = 1e18;
    for(const tagResource& r : info.resources){
        if(r.Type != RESOURCE_BUSH && r.Type != RESOURCE_GAZELLE) continue;
        bool isBush = (r.Type == RESOURCE_BUSH);
        bool isCorpse = (r.Type == RESOURCE_GAZELLE) && r.Blood <= 0;
        if(( isBush || isCorpse) && takenSource.count(r.SN)) continue;   // 尸体已被占
        if(inEnemyZone(r.DR,r.UR)) continue;
        double dist = calDistance(dr,ur,r.DR,r.UR);
        if(dist < bestDis){ bestDis = dist; bestSN = r.SN; }
    }
    return bestSN;
}

int findNearestResource(const tagInfo& info,int resType,double dr,double ur,int selfSN)
{
    const bool woodCrowd = (resType == RESOURCE_TREE);        //  只对"树"做拥挤过滤
    for(int pass = 0; pass < (woodCrowd ? 2 : 1); pass++){    // 木头两趟：先躲拥挤，找不到再放宽
        const bool useCrowd = woodCrowd && (pass == 0);
        int bestSN = -1;
        double bestDis = 1e18;
        for(const tagResource& r : info.resources){
            if(r.Type != resType) continue;
            if(inEnemyZone(r.DR,r.UR)) continue;
            if(takenSource.count(r.SN)) continue;
            // 如果以后加了"卡死拉黑"，把下面两行打开：
            // auto bn = resBanUntil.find(r.SN);
            // if(bn != resBanUntil.end() && bn->second > gameFrame) continue;

            if(useCrowd){                                     // ★ 只有木头会进这里
                bool crowded = false;
                for(const tagFarmer& g : info.farmers){
                    if(g.SN == selfSN) continue;              // 自己不算
                    if(g.WorkObjectSN < 0) continue;          // 只看正在干活/正走过去的
                    if(calDistance(g.DR, g.UR, r.DR, r.UR) <= 1.5 * BLOCKSIDELENGTH){
                        crowded = true; break;                // 别人已经贴着这棵树了 → 换一棵
                    }
                }
                if(crowded) continue;
            }

            double dist = calDistance(dr,ur,r.DR,r.UR);
            if(dist < bestDis){ bestDis = dist; bestSN = r.SN; }
        }
        if(bestSN >= 0) return bestSN;                        // 这一趟找到就返回
    }
    return -1;
}

bool inEnemyZone(double dr, double ur)
{
    if(enemyDR < 0 || enemyUR < 0) return false;      // 还没定位敌人锚点 → 不限制
    double d = calDistance(dr, ur,
                           enemyDR * BLOCKSIDELENGTH,
                           enemyUR * BLOCKSIDELENGTH);
    return d <= enemySafeBlocks * BLOCKSIDELENGTH;
}

int countBuilding(const tagInfo& info,int type)
{
    int cnt = 0;
    for(const tagBuilding& b : info.buildings){
        if(b.Type == type && b.Percent >= 100) cnt++;
    }
    return cnt;
}

bool findBuildSite(const tagInfo& info, int buildingType,int myDR,int myUR, int& outDR, int& outUR)
{
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
                if(canBuildHere(info,dr,ur,size) && !overlapsBuilding(info,dr,ur,size) && !unitOnSite(info,dr,ur,size)){
                    outDR = dr;
                    outUR = ur;
                    return true;
                }
            }
        }
    }
    return false;        
}

int buildBuildingSize(int type)
{
    if(type == BUILDING_HOME || type == BUILDING_ARROWTOWER) return 2;
    return 3;
}

bool canBuildHere(const tagInfo& info,int dr,int ur,int size)
{
    if(info.theMap == nullptr) return false;
    const vector<vector<tagTerrain>>& tm = *info.theMap;
    const int limDR = (int)tm.size();
    if(limDR <= 0) return false;
    const int limUR = (int)tm[0].size();
    if(dr < 0 || ur < 0 || dr + size > limDR || ur + size > limUR) return false;

    const int h0 = tm[dr][ur].height;
    if(h0 == -1) return false;                                   // 地基角落在海里

    for(int i = 0; i < size; i++){
        for(int j = 0; j < size; j++){
            const tagTerrain& t = tm[dr+i][ur+j];
            if(t.type == MAPPATTERN_UNKNOWN) return false;       //  未探索 → 换一格
            if(t.type == MAPPATTERN_OCEAN)   return false;       // 海/河
            if(t.height != h0)               return false;       // 高度不一致 = 斜坡
        }
    }
    return true;
}

bool overlapsBuilding(const tagInfo& info, int dr, int ur, int size)
{
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

bool unitOnSite(const tagInfo& info, int dr, int ur, int size)
{
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



void manageBuild(const tagInfo& info)
{
    if(buildSN >= 0){
        int hisWork = -1;
        for(const tagFarmer& f : info.farmers)
            if(f.SN == buildSN){ hisWork = f.WorkObjectSN; break; }
        if(hisWork >= 0)
            for(const tagBuilding& b : info.buildings)
                if(b.SN == hisWork && b.Percent < 100) return;      // 正在建 → 一个字都不下
    }
    // 建造优先级：房 -> 市民 -> 兵营 -> 箭塔
    int wantType = -1;
    if(countBuilding(info,BUILDING_HOME) < 5) wantType = BUILDING_HOME;
    else if(countBuilding(info,BUILDING_MARKET) < marketTarget) wantType = BUILDING_MARKET;
    else if(countBuilding(info,BUILDING_ARMYCAMP) < campTarget) wantType = BUILDING_ARMYCAMP;
    else if(needTower&& countBuilding(info, BUILDING_ARROWTOWER) < towerTarget){
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
    else if(countBuilding(info,BUILDING_HOME) < 13) wantType = BUILDING_HOME;
    if(wantType >=0){
        int buildDR = -1;
        int buildUR = -1;
        int foundationSN = -1;
        for(const tagBuilding& b: info.buildings){
            if(b.Type == wantType && b.Percent < 100){
                foundationSN = b.SN;
                break;
            }
        }
        if(foundationSN<0){
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
                    return;
                }
            }
        }
        for(const tagFarmer& f : info.farmers){
            if(buildSN < 0) return;
            int hisWork = -1;
            for(const tagFarmer& f : info.farmers){
                if(f.SN == buildSN){ hisWork = f.WorkObjectSN; break; }
            }
            if(foundationSN >= 0){                       // 已有地基/工地 → 让他接着建
                if(hisWork != foundationSN) HumanAction(buildSN, foundationSN);
            }else{                                       // 没有地基 → 下新建令
                HumanBuild(buildSN, wantType, buildDR, buildUR);
                DbgBuild("建建筑", buildSN, wantType, buildDR, buildUR);
            }
            return;
        }
    }
}

void manageResearch(const tagInfo& info){
    /*for(const tagBuilding& b : info.buildings){
        if(b.Project == ACT_NULL) continue;
        switch(b.Project){
            case BUILDING_GRANARY_ARROWTOWER:
            case BUILDING_RANGE_UPGRADE_COMPOSITE_BOW:
            case BUILDING_MARKET_WOOD_UPGRADE:
            case BUILDING_MARKET_CRAFT_UPGRADE:
            case BUILDING_STOCK_UPGRADE_USETOOL:
            case BUILDING_STOCK_UPGRADE_DEFENSE_ARCHER:
            case BUILDING_MARKET_FARM_UPGRADE:
                techDone.insert(b.Project); break;      // 造兵/造农民/升时代不记
            default: break;
        }
    }*/
    if(!arrowTechOrdered) arrowTechOrdered = true;   // 真开始研发了才算数

    // ③ 经济科技
    researchIfPossible(info,BUILDING_MARKET ,BUILDING_MARKET_WOOD_UPGRADE   ,120, 75,0,0);  // 伐木：采集+50%
    researchIfPossible(info,BUILDING_GRANARY,BUILDING_GRANARY_ARROWTOWER   ,120,  0,50,0);  // 箭塔科技
    researchIfPossible(info,BUILDING_STOCK  ,BUILDING_STOCK_UPGRADE_USETOOL,100,  0,0,0);   // 工具使用

    // ④ 复合弓科技：资源够 且 已进入造兵阶段（青铜时代 + 靶场已建好）
    bool armyStage = (info.civilizationStage >= CIVILIZATION_BRONZEAGE);
    if(armyStage)
        researchIfPossible(info,BUILDING_RANGE,BUILDING_RANGE_UPGRADE_COMPOSITE_BOW,180,100,0,0);
        // ===== 复合弓兵增益：射程 +1/+1、弓兵护甲 +2/+2 =====
    //if(techDone.count(BUILDING_RANGE_UPGRADE_COMPOSITE_BOW) > 0){
        researchIfPossible(info,BUILDING_MARKET,BUILDING_MARKET_WOOD_UPGRADE ,120, 75,0,0);  // 射程+1
        researchIfPossible(info,BUILDING_MARKET,BUILDING_MARKET_CRAFT_UPGRADE,170,150,0,0); // 射程再+1（需伐木加工）
        researchIfPossible(info,BUILDING_STOCK ,BUILDING_STOCK_UPGRADE_DEFENSE_ARCHER,100,0,0,0); // 护甲+2
    //}
}

bool researchIfPossible(const tagInfo& info,int buildingType,int action,
         int needFood,int needWood,int needStone,int needGold)
{
            if(info.Meat < needFood || info.Wood < needWood || info.Stone < needStone
               || info.Gold < needGold) return false;
            for(const tagBuilding& b : info.buildings){
                if(b.Type == buildingType && b.Percent >=100 && b.Project == ACT_NULL){
                    BuildingAction(b.SN,action);
                    DbgAct("研发科技", b.SN, action);
                    if(action == BUILDING_RANGE_UPGRADE_COMPOSITE_BOW) rangeTarget = 3;   // 造兵阶段：靶场升级后就可以造复合弓兵了
                    return true;
                }
            }
            return false;         
}

void manageScout(const tagInfo& info)
{
    const double B = BLOCKSIDELENGTH;
    const double hx = (myarrowDR >= 0 ? myarrowDR : mycenterDR) * B;
    const double hy = (myarrowUR >= 0 ? myarrowUR : mycenterUR) * B;

    if(priestSN < 0){
        for(const tagArmy& a : info.armies)
            if(a.Sort == AT_PRIEST){ priestSN = a.SN; break; }
    }
    if(priestSN < 0) return;

    // ===== 时间到 → 回家（只切一次状态） =====
    if(phase != PHASE_ECON && (scoutState == SCOUT_OUT || scoutState == SCOUT_REST)){
        scoutState = SCOUT_RETURN;
        scoutTgtDR = -1; scoutTgtUR = -1;
        HumanMove(priestSN, hx, hy);
        lastOrderFrame = gameFrame;
    }

    // ===== 回家 =====
    if(scoutState == SCOUT_RETURN){
        double d = 1e18; bool gotP = false;
        for(const tagArmy& p : info.armies){
            if(p.SN != priestSN) continue;
            d = calDistance(p.DR, p.UR, hx, hy); gotP = true; break;
        }
        if(!gotP) return;
        if(d < 3*B){                                    // 到家
            if(phase != PHASE_ECON) scoutState = SCOUT_DONE;
            else { homeRestUntil = gameFrame + 25; scoutState = SCOUT_REST; }
            scoutTgtDR = -1; scoutTgtUR = -1; stillCnt = 0;
            return;
        }
        if(gameFrame - lastOrderFrame >= 25){
            HumanMove(priestSN, hx, hy);
            lastOrderFrame = gameFrame;
        }
        return;
    }
    if(scoutState == SCOUT_DONE) return;

    // ===== 在家休息完 → 继续探路 =====
    if(scoutState == SCOUT_REST){
        if(gameFrame < homeRestUntil) return;
        if(!netReady) buildScoutNet(info);
        netCur = -1; netNext = 0;
        scoutTgtDR = -1; scoutTgtUR = -1;
        scoutState = SCOUT_OUT;
        return;
    }
    scoutState = SCOUT_OUT;                             // 兜底：任何非法状态都当 OUT

    // ===== 探路 =====
    if(!netReady || (int)netL.size() < 10) buildScoutNet(info);   // 点太少 = 无效，重建
    if(netL.empty()) return;
    if(netL.empty()) return;
    const int NP = (int)netL.size();
    if((int)netDone.size() < NP) netDone.assign(NP, 0);

    // ===== 先定位祭司（危险判断要用他自己的位置）=====
    double pdr = -1, pur = -1; bool got = false;
    for(const tagArmy& p : info.armies){
        if(p.SN != priestSN) continue;
        pdr = p.DR; pur = p.UR; got = true; break;
    }
    if(!got) return;

    // ===== 看到敌人：①拉黑敌人周围路点 ②祭司自己被逼近就当场丢目标 =====
    const double BLACK_R  = 25.0;        // 敌人周围 25 格内的路点作废（单位：格）
    const double DANGER_R = 14.0 * B;    // 敌人在祭司 14 格内 = 危险（世界坐标）
    bool danger = false;
    for(const tagArmy& e : info.enemy_armies){
        if(e.Blood <= 0) continue;
        const double edr = e.DR / B, eur = e.UR / B;
        for(int i = 0; i < NP; i++)
            if(!netDone[i] && calDistance(netL[i], netU[i], edr, eur) < BLACK_R) netDone[i] = 1;
        if(calDistance(pdr, pur, e.DR, e.UR) < DANGER_R) danger = true;
    }
    if(danger && scoutTgtDR >= 0){                     // ★ 一看到就规避：马上放弃当前点
        scoutTgtDR = -1; scoutTgtUR = -1; stillCnt = 0;
    }
    if(scoutTgtDR >= 0 && netCur >= 0 && netCur < NP && netDone[netCur]){
        scoutTgtDR = -1; scoutTgtUR = -1; stillCnt = 0;   // 目标点本身被拉黑（原有逻辑）
    }

    // 卡住检测：只看"位置有没有变"，与时间无关
    if(fabs(pdr - lastPosDR) < 0.5 && fabs(pur - lastPosUR) < 0.5) stillCnt++;
    else { stillCnt = 0; lastPosDR = pdr; lastPosUR = pur; }

    // 换点判据：只有 ①到达 ②真的没动
    if(scoutTgtDR >= 0){
        double dw = calDistance(pdr, pur, scoutTgtDR, scoutTgtUR);
        if(dw < 3*B){                                   // ① 到达
            scoutTgtDR = -1; scoutTgtUR = -1; stillCnt = 0;
        }else if(stillCnt >= 4){                        // ② 卡住（无任何定时）
            if(netCur >= 0 && netCur < NP) netDone[netCur] = 1;
            scoutTgtDR = -1; scoutTgtUR = -1; stillCnt = 0;
        }
    }

    // 选新点：环形推进，走完一轮自动重来
    if(scoutTgtDR < 0){
        int pick = -1;
        for(int k = 0; k < NP; k++){
            int i = (netNext + k) % NP;
            if(!netDone[i]){ pick = i; break; }
        }
        if(pick < 0){
            for(int i = 0; i < NP; i++) netDone[i] = 0;
            pick = netNext % NP;
        }
        netCur  = pick;
        netNext = (pick + 1) % NP;
        scoutTgtDR = netL[pick]*B + B/2;
        scoutTgtUR = netU[pick]*B + B/2;
        stillCnt = 0; lastPosDR = pdr; lastPosUR = pur;
        lastOrderFrame = gameFrame;
        HumanMove(priestSN, scoutTgtDR, scoutTgtUR);
        return;
    }

    // 同一目标点：25 帧重发一次（绝不在这里换点）
    if(gameFrame - lastOrderFrame >= 25){
        HumanMove(priestSN, scoutTgtDR, scoutTgtUR);
        lastOrderFrame = gameFrame;
    }
}

void buildScoutNet(const tagInfo& info)
{
    netL.clear(); netU.clear(); netDone.clear();
    netCur = -1; netNext = 0; netReady = false;
    if(info.theMap == nullptr || mycenterDR < 0 || enemyDR < 0) return;

    const double STEP = 10.0;        // ★ 每 10 格一个路点

    // 自家角 = 离基地最近的地图角；"上方角" = 同一条边另一头；"对角" = 两个坐标都翻过去
    const double E = 10.0;                                       // ★ 离地图边缘 10 格
    const int cx = (mycenterDR * 2 >= MAP_L) ? 1 : 0;
    const int cy = (mycenterUR * 2 >= MAP_U) ? 1 : 0;
    const double p0L = cx ? (MAP_L - 1 - E) : E;                 // ① 自家角
    const double p0U = cy ? (MAP_U - 1 - E) : E;
    const double p1L = p0L;                                      // ② 自家上方的角
    const double p1U = cy ? E : (MAP_U - 1 - E);
    const double p2L = cx ? E : (MAP_L - 1 - E);                 // ③ 上方角的对角
    const double p2U = p0U;
    const double p4L = (mycenterDR + MAP_L / 2.0) / 2.0;         // ④ 自家与地图中心的中点
    const double p4U = (mycenterUR + MAP_U / 2.0) / 2.0;
    const double lo = 2.0;
    auto clampL = [&](double v){ return v < lo ? lo : (v > MAP_L - 3.0 ? MAP_L - 3.0 : v); };
    auto clampU = [&](double v){ return v < lo ? lo : (v > MAP_U - 3.0 ? MAP_U - 3.0 : v); };

    // 沿一条线段每 STEP 格撒一个点；withEnd=false 时不含终点（避免和下一段起点重复）
    auto addSeg = [&](double l0, double u0, double l1, double u1, bool withEnd){
        const double dl = l1 - l0, du = u1 - u0;
        const double len = sqrt(dl*dl + du*du);
        if(len < 1e-6) return;
        int n = (int)(len / STEP + 0.5);
        if(n < 1) n = 1;
        for(int i = 0; i <= n; i++){
            if(i == n && !withEnd) break;
            const double t = double(i) / double(n);
            netL.push_back(clampL(l0 + dl*t));
            netU.push_back(clampU(u0 + du*t));
        }
    };

    addSeg(p0L, p0U, p1L, p1U, false);   // ① 自家角 → 自家上方的角
    addSeg(p1L, p1U, p2L, p2U, false);   // ② 上方角 → 上方角的对角（跨全场的斜线）
    addSeg(p2L, p2U, p0L, p0U, false);   // ③ 对角 → 回到自家角
    addSeg(p0L, p0U, p4L, p4U, true);    // ④ 自家角 → 自家与地图中心的中点（含终点）

    if(netL.empty()) return;
    netDone.assign(netL.size(), 0);
    netReady = true;
}
void manageArmyKite(const tagInfo &info)
{
    int nBow = 0, nThrow = 0;
    for(const tagArmy& a : info.armies){
        if(a.Sort == AT_COMPOSITE_BOWMAN)   nBow++;
        else if(a.Sort == AT_STONE_THROWER) nThrow++;
    }
    // 兵种清洗：留复合弓兵 / 投石车 / 祭司，其余自杀
    if(info.armies.size()>10&&info.GameFrame>25000){
        
        for(const tagArmy& a : info.armies){
            if(a.SN == priestSN) continue;
            if(a.Sort == AT_COMPOSITE_BOWMAN || a.Sort == AT_STONE_THROWER) continue;
            HumanAction(a.SN, a.SN);                                     // 自毁
        }
    }
    if(!isArmyReady && nBow+nThrow >= wantBowman-15) isArmyReady = true;           // 弓兵成型 → 转入反攻
    // 科技：复合弓（180 食 + 100 木）
    if(nBow < wantBowman&&info.civilizationStage>=CIVILIZATION_BRONZEAGE)
        researchIfPossible(info, BUILDING_RANGE, BUILDING_RANGE_UPGRADE_COMPOSITE_BOW, 180, 100, 0, 0);
    if(homeTarget<13 && info.farmers.size()+info.armies.size() >= (homeTarget)*4) homeTarget++;   // 兵营未满员 → 不造复合弓兵
    if(homeTarget>=8) rangeTarget = 3;
    if(nBow>=3) rangeTarget = 3; 
    // 造复合弓兵（40 食 + 20 金）
    if(nBow+nThrow <30 
       && info.Meat >= BUILDING_RANGE_CREATE_COMPOSITE_BOWMAN_FOOD
       && info.Gold >= BUILDING_RANGE_CREATE_COMPOSITE_BOWMAN_GOLD){
        for(const tagBuilding& b : info.buildings){
            if(b.Type != BUILDING_RANGE) continue;
            if(b.Percent < 100 || b.Project != ACT_NULL) continue;
            DbgAct("生产复合弓兵", b.SN, BUILDING_RANGE_CREATE_COMPOSITE_BOWMAN);
            BuildingAction(b.SN, BUILDING_RANGE_CREATE_COMPOSITE_BOWMAN);
            break;
        }
    }
}

void manageAttackKite(const tagInfo &info)
{
    if(!isArmyReady) return;
    const double B = BLOCKSIDELENGTH;
    int siegeSN = -1;
    int etowerSN = -1, etowerBL = -1, etowerBU = -1; double bestTd = 1e18;
    int etowerNum=-1;
    for(const tagBuilding& b : info.enemy_buildings){
        if(b.Type == BUILDING_SIEGE){
            siegeSN=b.SN;
            continue;
        }
        etowerNum += etowerNum==-1?2:1;
        double d = calDistance(b.BlockDR, b.BlockUR, mycenterDR, mycenterUR);
        if(d < bestTd){ bestTd = d; etowerSN = b.SN; etowerBL = b.BlockDR; etowerBU = b.BlockUR; }
    }
    if(info.enemy_armies.empty()&&siegeSN>=0&&etowerNum<=2){
        for(const tagArmy& p:info.armies){
            if(p.SN != priestSN) continue;
            if(p.WorkObjectSN!=siegeSN){
                HumanAction(priestSN,siegeSN);
            }
            break;
        }
    }else{
        for(const tagArmy& p:info.armies){
            if(p.SN != priestSN) continue;
            HumanMove(p.SN,50*B,50*B);
            break;
        }
    }
    if(info.enemy_armies.empty()&&siegeSN>=0&&etowerNum==-1){
        for(const tagArmy& a: info.armies){
            if(a.SN == priestSN) continue;
            if(a.WorkObjectSN==-1){
                HumanMove(a.SN,50*B,50*B);
            }
        }
    }
    for(const tagArmy& a:info.armies){
        if(a.SN == priestSN) continue;
        if(a.Sort != AT_COMPOSITE_BOWMAN && a.Sort !=AT_STONE_THROWER) continue;
        if(inList(rallyGroup,a.SN)) continue;
        if(info.enemy_armies.empty()&&etowerNum==-1&&siegeSN==-1){
            HumanMove(a.SN,enemyDR*B,enemyUR*B);
            continue;
        }
        if(!info.enemy_armies.empty()){
            const double backR  = (a.Sort == AT_STONE_THROWER) ? 8.0 : 5.0;
            
            int batterSN = -1;
            double batterD = 1e18;
            const double batterR = (a.Sort == AT_STONE_THROWER)
                                 ? (double(DIS_STONE_THROWER) + 2.0)      // 投石车：10+2 = 12 格
                                 : (double(DIS_COMPOSITE_BOWMAN) + 1.0);  //  弓兵：7+1 = 8 格
            for(const tagArmy& e : info.enemy_armies){
                if(e.Blood <= 0) continue;
                if(e.Sort != AT_STONE_THROWER) continue;                  // 只盯对面投石车
                double d = calDistance(a.DR, a.UR, e.DR, e.UR);
                if(d <= batterD){ batterD = d; batterSN = e.SN; }
            }
            if(batterSN >= 0 && batterD > batterR * B) batterSN = -1; 

            int tgtSN = -1;
            double tgtD = 1e18;
            double tdr = 0;
            double tur = 0;
            for(const tagArmy& e : info.enemy_armies){
                double d = calDistance(a.DR, a.UR, e.DR, e.UR);
                if(d < tgtD){ tgtD = d; tgtSN = e.SN; tdr = e.DR; tur = e.UR; }
            }
            if(tgtSN>=0&&tgtD<=backR*B){
                HumanMove(a.SN,mycenterDR*B,mycenterUR*B);
            }else if(batterSN>=0 && batterD<=batterR*B){
                if(a.WorkObjectSN!=batterSN)
                    HumanAction(a.SN,batterSN);
            }else if(tgtSN>=0 && tgtD>backR*B){
                if(a.WorkObjectSN!=tgtSN)
                    HumanAction(a.SN,tgtSN);
            }
        }else if(etowerNum>0){
                if(a.WorkObjectSN!=etowerSN)
                  HumanAction(a.SN,etowerSN);
        }
    }
}

void manageDefense(const tagInfo &info)
{
    if(info.GameFrame>25000&&isArmyReady) return;
    double hx, hy;
    for(const tagArmy& a : info.armies){
        if(a.Sort == AT_PRIEST){
            priestSN = a.SN;
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
           if(calDistance(e.DR, e.UR, hx, hy) < 30 * BLOCKSIDELENGTH){ threat = true; break; }
        }
        double xx = (myarrowDR >= 0 ? myarrowDR : mycenterDR) * BLOCKSIDELENGTH;
        double yy = (myarrowUR >= 0 ? myarrowUR : mycenterUR) * BLOCKSIDELENGTH;
        if(calDistance(xx, yy, hx, hy) < 10 * BLOCKSIDELENGTH) threat = true;
        if(!threat) return;
    }
    
    bool haveArrowTower = false;
    for(const tagBuilding& b : info.buildings){
        if(b.Type == BUILDING_ARROWTOWER && b.Percent>=100){
            haveArrowTower = true;
            towerSN = b.SN;
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
                   if(e.WorkObjectSN != towerSN ){
                        attackTgt = e.SN;
                        if(e.Sort == AT_STONE_THROWER){
                            attackTgt = e.SN;
                            break;
                        }
                    }
                }
            }
            if(attackTgt>=0){
                HumanAction(towerSN,attackTgt);
                continue;
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
    bool fff=true; 
    for(const tagArmy& p : info.armies){
        if(p.SN != priestSN) continue;
        double xx = (myarrowDR >= 0 ? myarrowDR : mycenterDR) * BLOCKSIDELENGTH;
        double yy = (myarrowUR >= 0 ? myarrowUR : mycenterUR) * BLOCKSIDELENGTH;
        xx=(xx+mycenterDR*BLOCKSIDELENGTH)/2;
        yy=(yy+mycenterUR*BLOCKSIDELENGTH)/2;
        if(p.WorkObjectSN >= 0 && calDistance(p.DR, p.UR, xx, yy) < 15 * BLOCKSIDELENGTH) break;
        if(calDistance(p.DR, p.UR, xx, yy) > 5 * BLOCKSIDELENGTH){
           HumanMove(priestSN, xx, yy);
           fff=false;
           break;
        }
    }
    if(fff){
        for(const tagArmy& e: info.enemy_armies){
            if(calDistance(e.DR, e.UR, hx, hy) < 20 * BLOCKSIDELENGTH){
                fff=true;
                break;
            }
            fff=false;
        }
    }
    if(fff){
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
                for(const tagArmy& p : info.armies){
                    if(p.Sort == AT_PRIEST){
                    if(p.WorkObjectSN != convertTarget) convertTarget = -1;
                    break;
                    }
                }
            }
        }
        if(convertTarget < 0){
            bool ff=false;
            for(const tagArmy& p : info.armies){
                if(p.Sort == AT_PRIEST){
                  if(p.ConvertCooldown <= 0) ff=true;
                  else{
                        double xx = (myarrowDR >= 0 ? myarrowDR : mycenterDR) * BLOCKSIDELENGTH;
                        double yy = (myarrowUR >= 0 ? myarrowUR : mycenterUR) * BLOCKSIDELENGTH;
                        xx=(xx+mycenterDR*BLOCKSIDELENGTH)/2;
                        yy=(yy+mycenterUR*BLOCKSIDELENGTH)/2;
                        HumanMove(priestSN, xx, yy);
                        break;
                    }
                }
            }
            if(ff){
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
                }
            }
        }
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
            if(eSN >= 0 && eD <= PULL*B ){
                double rx, ry;                              
                if(calDistance(a.DR,a.UR,mycenterDR*B,mycenterUR*B)>5*B){
                    rx = mycenterDR*B + B/2;
                    ry = mycenterUR*B + B/2;
                }else{
                    rx = enemyDR*B;
                    ry = enemyUR*B;
                }
                
                HumanMove(a.SN, rx, ry);
                continue;                                         // 这一拍不派攻击目标
            }
        }
        bool curIsEnemy = false;
        for(const tagArmy& e : info.enemy_armies){
            if(e.SN == a.WorkObjectSN){ curIsEnemy = true; break; }
        }
        if(curIsEnemy) continue;
        int tgt= -1;
        int bestD = 1000000;
        for(const tagArmy& e:info.enemy_armies){
            if(e.Sort == AT_STONE_THROWER) continue;
            int ddr = abs(a.DR - e.DR), dur = abs(a.UR - e.UR);
            int d = ddr*ddr + dur*dur;
            if(d < bestD){ bestD = d; tgt = e.SN; }
        }
        if(tgt>=0) HumanAction(a.SN,tgt);
    }

    if(!haveArrowTower){
        for(const tagArmy& a: info.armies){
            if(a.SN == priestSN){
                continue;
            }
            for(const tagArmy& e: info.enemy_armies){
                if(e.WorkObjectSN==priestSN){
                   HumanAction(a.SN,e.SN);
                   break;
                }
                  
            }
            break;
        }
    }
}
void manageUpdate(const tagInfo &info)
{ 
    if(info.civilizationStage < CIVILIZATION_BRONZEAGE && info.Meat >=810){
        for(const tagBuilding& b : info.buildings){
            if(b.Type == BUILDING_CENTER){
                if(b.Project == ACT_NULL){
                    //researchIfPossible(info,BUILDING_MARKET,BUILDING_MARKET_FARM_UPGRADE,200,50,0,0);
                    BuildingAction(b.SN,BUILDING_CENTER_UPGRADE);
                    DbgAct("升时代", b.SN, BUILDING_CENTER_UPGRADE);
                }
                break;
            }
        }
    }
}

// ===== 反攻集结 =====
void manageRally(const tagInfo& info)
{
    const double B  = BLOCKSIDELENGTH;
    const double gx = (MAP_L / 2.0) * B;          // (50,50)
    const double gy = (MAP_U / 2.0) * B;
    const double ARRIVE = 6 * B;

    // 1) 把死掉的从集结队列剔掉
    for(size_t i = 0; i < rallyGroup.size(); ){
        bool alive = false;
        for(const tagArmy& a : info.armies) if(a.SN == rallyGroup[i]){ alive = true; break; }
        if(alive) ++i; else rallyGroup.erase(rallyGroup.begin() + i);
    }

    // 2) 新兵入列（没在集结、也没参战过的弓兵/投石车）
    for(const tagArmy& a : info.armies){
        if(a.SN == priestSN) continue;
        if(a.Sort != AT_COMPOSITE_BOWMAN && a.Sort != AT_STONE_THROWER) continue;
        if(inList(rallyGroup, a.SN)) continue;
        if(rallyDone.count(a.SN)) continue;
        rallyGroup.push_back(a.SN);               //  新兵自动进本批
    }

    const int need = (rallyBatch == 0) ? 15 : 5;  //  第一批 15，之后每批 5

    // 3) 到齐 → 整批放出去
    vector<int> readySN;
    for(size_t i = 0; i < rallyGroup.size(); i++){
        if(rallyDone.count(rallyGroup[i])) continue;      // 已经放出去的不算
        double d = 1e18;
        for(const tagArmy& a : info.armies)
            if(a.SN == rallyGroup[i]){ d = calDistance(a.DR, a.UR, gx, gy); break; }
        if(d <= ARRIVE) readySN.push_back(rallyGroup[i]);
    }
    // 4) 
    if((int)readySN.size() >= need){
        for(int i = 0; i < need; i++){
            rallyDone.insert(readySN[i]);                 // 加入战斗
            std::cerr << "[rally] 第 " << (rallyBatch+1) << " 批出发 f=" << gameFrame
                      << " SN=" << readySN[i] << " (" << (i+1) << "/" << need << ")" << std::endl;
        }
        rallyBatch++;
    }

    // 4) 没到齐 → 每 25 帧催一次（别每帧下，会把路径清空）
    for(size_t i = 0; i < rallyGroup.size(); i++){
        if(rallyDone.count(rallyGroup[i])) continue;
        for(const tagArmy& a : info.armies){
            if(a.SN != rallyGroup[i]) continue;
            if(calDistance(a.DR, a.UR, gx, gy) > 8 * B) HumanMove(a.SN, gx, gy);
            break;
        }
    }
}