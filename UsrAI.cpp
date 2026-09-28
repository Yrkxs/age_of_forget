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
    int wantFood  = 8;      // 采食（浆果+羚羊）
    int wantWood  = 8;      // 伐木
    int wantStone = 1;      // 采石
    int wantGold  = 2;      // 采金
    int wantBuild = 1;      // 建筑工（不建仓库/谷仓）
    // ===== 临时建仓库/谷仓 =====
    map<int,int> depotJob;      // 农民SN -> 原任务
    map<int,int> depotJobPos;   // 农民SN -> 工地块(dr*1000+ur)
    int depotRange   = 20;           // 多少格内没有仓库/谷仓才去建
    int depotRichCnt = 150;          // 资源"挺多"的门槛
    double enemySafeBlocks = 30.0;

    void manageFarmer(const tagInfo& info); 
    int findNearestFood(const tagInfo& info,double dr,double ur,const std::map<int,int>& takenSource);
    int findNearestResource(const tagInfo& info,int resType,double dr,double ur);
    bool findBuildSite(const tagInfo& info, int buildingType,int myDR,int myUR, int& outDR, int& outUR);
    int buildBuildingSize(int type);
    bool canBuildHere(const tagInfo &info, int dr, int ur);
    bool overlapsBuilding(const tagInfo &info, int dr, int ur, int size);
    bool unitOnSite(const tagInfo &info, int dr, int ur, int size);
    bool inEnemyZone(double dr, double ur);
    int countBuilding(const tagInfo& info,int type);

    // manageBuild()
    int marketTarget = 1;
    int rangeTarget=3;
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
    std::set<int> techDone;

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

void UsrAI::processData()
{   
//#################################################################################################
    g_ai = nullptr;  
    g_ai = this;
    tagInfo info = getInfo();  //本帧快照
    gameFrame = info.GameFrame;
    getPhase(gameFrame);
    if(gameFrame%50!=0) return;

     //找我方市镇中心\箭塔和对方大概来向
    manageInit(info);
    
    // 造村民
    manageProduction(info);
    
    // 任务管理（采集+建造）
    manageFarmer(info);

    manageBuild(info);

    // 探路
    manageScout(info);

    // 科技研发
    manageResearch(info);

    // 造兵
    manageArmyKite(info);

    // 反攻
    if(isArmyReady) manageAttackKite(info);

    // 防御
    if(phase!=PHASE_ECON) manageDefense(info);

    // 升级时代
    manageUpdate(info);
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
        return;
    }
}



// 任务号：1食 2木 3石 4金 5田 6建筑工（7=临时建仓库，不写进 humantask，用 depotJob 记）
void manageFarmer(const tagInfo& info)
{
    const double B = BLOCKSIDELENGTH;
    const double refDR = (mycenterDR >= 0) ? mycenterDR*B : 50*B;   //  找资源一律以市中心为基准
    const double refUR = (mycenterUR >= 0) ? mycenterUR*B : 50*B;
    takenSource.clear();

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
    for(const tagBuilding& b:info.buildings){
        if(b.Type==BUILDING_ARROWTOWER) towernn++;
        else if(b.Type==BUILDING_RANGE) r++;
    }
    if(towernn>=3) needTower=false;
    if(r>=3){
        homeup=true;
    }
    const bool bronze = (r>=3);
    int tFood  = bronze ? 0 : wantFood;
    int tFarm  = bronze ? wantFood : 0;
    int tWood  = wantWood, tStone = wantStone, tGold = wantGold, tBuild = wantBuild;
    
    if(wantFood==8){
        if(towernn*150+info.Wood>=450){
            tStone = 0;
            wantFood+=2;
        }
    }

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
                    moved = true; break;
                }
            if(!moved) break;
        }

    }
    // 3b. 缺编 → 用空闲的人补（顺序：金→石→木→食→田）
    static const int prio[6] = { TASK_BUILD, TASK_STONE, TASK_FOOD, TASK_GOLD, TASK_WOOD, TASK_FARM };
    for(int k = 0; k < 6; k++){
        int t = prio[k];
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
        if(handOk) continue;

        // ---- 4.2 种田（升级后才有人）----
        if(t == TASK_FARM){
            if(info.Wood < BUILD_FARM_WOOD||(info.civilizationStage>=CIVILIZATION_BRONZEAGE&& !techDone.count(BUILDING_RANGE_UPGRADE_COMPOSITE_BOW))){
                if(f.NowState != HUMAN_STATE_IDLE) continue;
                // 木料不够 → 先去砍木（木头一够，下一拍就会走下面的"种田"分支）
                int rsn = findNearestResource(info, RESOURCE_TREE, refDR, refUR);
                if(rsn >= 0){ takenSource[rsn] = 1; HumanAction(f.SN, rsn); }
                continue;
            }
            // 木料够 → ① 有种好的空田就去种 ② 否则建一块田
            int freeFarm = -1; double bestD = 1e18;
            for(const tagBuilding& b : info.buildings){
                if(b.Type != BUILDING_FARM || b.Percent < 100) continue;
                bool used = false;
                for(const tagFarmer& g : info.farmers)
                    if(g.WorkObjectSN == b.SN){ used = true; break; }
                if(used) continue;
                double dd = calDistance(f.DR, f.UR, b.BlockDR*B, b.BlockUR*B);
                if(dd < bestD){ bestD = dd; freeFarm = b.SN; }
            }
            if(freeFarm >= 0){ HumanAction(f.SN, freeFarm); continue; }
            int bd = -1, bu = -1;
            if(findBuildSite(info, BUILDING_FARM, myfameDR, myfameUR, bd, bu)){
                HumanBuild(f.SN, BUILDING_FARM, bd, bu);
            }else{
                // 找不到地基 → 也别站着，去采食/砍木
                int rsn = findNearestResource(info, RESOURCE_BUSH, refDR, refUR);
                if(rsn < 0) rsn = findNearestResource(info, RESOURCE_TREE, refDR, refUR);
                if(rsn >= 0){ takenSource[rsn] = 1; HumanAction(f.SN, rsn); }
            }
            continue;
        }

        // ---- 4.3 采集类（食/木/石/金）：找一块"没人采"的资源 ----
        if(f.NowState != HUMAN_STATE_IDLE) continue;              // 在走过去/在干活 → 别插队
        int rt = (t == TASK_FOOD)  ? RESOURCE_BUSH  :
                 (t == TASK_WOOD)  ? RESOURCE_TREE  :
                 (t == TASK_STONE) ? RESOURCE_STONE : RESOURCE_GOLD;
        int rsn = findNearestResource(info, rt, refDR, refUR);
        if(rsn < 0 && t == TASK_FOOD)
            rsn = findNearestResource(info, RESOURCE_GAZELLE, refDR, refUR);   // 食物：浆果没了就找羚羊
        if(rsn >= 0){
            takenSource[rsn] = 1;
            humantaskType[f.SN] = t;
            HumanAction(f.SN, rsn);

            // ---- 4.4 这块资源"挺多"且 20 格内没有对应仓库/谷仓 →
            //          从本任务里派 1 个人去建（建完前不调走，且不占用建筑工）----
            bool needDepot = false; int bType = BUILDING_STOCK;
            double rDR = 0, rUR = 0; int rCnt = 0; int rType = -1;
            for(const tagResource& r : info.resources)
                if(r.SN == rsn){ rDR = r.DR; rUR = r.UR; rCnt = r.Cnt; rType = r.Type; break; }
            if(rCnt >= depotRichCnt){
                bType = (rType == RESOURCE_BUSH) ? BUILDING_GRANARY : BUILDING_STOCK;
                bool hasNear = false;
                for(const tagBuilding& b : info.buildings){
                    if(b.Type != bType) continue;
                    if(calDistance(b.BlockDR*B, b.BlockUR*B, rDR, rUR) <= depotRange*B){ hasNear = true; break; }
                }
                if(!hasNear) needDepot = true;
            }
            if(needDepot){
                bool already = false;
                for(const tagBuilding& b : info.buildings)           //已经有同类型的地基 不重复建
                    if(b.Type == bType && b.Percent < 100){ already = true; break; }
                if(!already){
                    int bd = -1, bu = -1;
                    if(findBuildSite(info, bType, (int)(rDR/B), (int)(rUR/B), bd, bu)){
                        depotJob[f.SN]    = t;                  // ★ 记住原任务
                        depotJobPos[f.SN] = bd*1000 + bu;
                        humantask[f.SN]   = t;                  // 名义任务不变
                        HumanBuild(f.SN, bType, bd, bu);
                    }
                }
            }
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

int findNearestResource(const tagInfo& info,int resType,double dr,double ur)
{
    int bestSN = -1;
    double bestDis = 1e18;
    for(const tagResource& r : info.resources){
        if(r.Type !=resType) continue;
        if(inEnemyZone(r.DR,r.UR)) continue;
        if(takenSource.count(r.SN)) continue;
        double dist = calDistance(dr,ur,r.DR,r.UR);
        if(dist < bestDis){
            bestDis = dist;
            bestSN = r.SN;
        }
    }
    return bestSN;
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
                if(canBuildHere(info,dr,ur) && !overlapsBuilding(info,dr,ur,size) && !unitOnSite(info,dr,ur,size)){
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

bool canBuildHere(const tagInfo& info,int dr,int ur)
{
    if(info.theMap == nullptr) return false;
    auto x = info.theMap->size();
    int limDR = (int)x;
    int limUR;
    if(limDR>0){
        auto y = (*info.theMap)[0].size();
        limUR = (int)y;
    }else{
        limUR = 0;
    }
    if(dr < 0 || ur < 0 || dr >= limDR || ur >= limUR) return false;
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
    if(countBuilding(info,BUILDING_HOME) < homeTarget) wantType = BUILDING_HOME;
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
            }
            return;
        }
    }
}

void manageResearch(const tagInfo& info){
    for(const tagBuilding& b : info.buildings){
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
    }
    if(!arrowTechOrdered && techDone.count(BUILDING_GRANARY_ARROWTOWER)) arrowTechOrdered = true;   // 真开始研发了才算数

    // ③ 经济科技
    researchIfPossible(info,BUILDING_MARKET ,BUILDING_MARKET_WOOD_UPGRADE   ,120, 75,0,0);  // 伐木：采集+50%
    researchIfPossible(info,BUILDING_GRANARY,BUILDING_GRANARY_ARROWTOWER   ,120,  0,50,0);  // 箭塔科技
    researchIfPossible(info,BUILDING_STOCK  ,BUILDING_STOCK_UPGRADE_USETOOL,100,  0,0,0);   // 工具使用

    // ④ 复合弓科技：资源够 且 已进入造兵阶段（青铜时代 + 靶场已建好）
    bool armyStage = (info.civilizationStage >= CIVILIZATION_BRONZEAGE);
    if(armyStage)
        researchIfPossible(info,BUILDING_RANGE,BUILDING_RANGE_UPGRADE_COMPOSITE_BOW,180,100,0,0);
        // ===== 复合弓兵增益：射程 +1/+1、弓兵护甲 +2/+2 =====
    if(techDone.count(BUILDING_RANGE_UPGRADE_COMPOSITE_BOW) > 0){
        researchIfPossible(info,BUILDING_MARKET,BUILDING_MARKET_WOOD_UPGRADE ,120, 75,0,0);  // 射程+1
        researchIfPossible(info,BUILDING_MARKET,BUILDING_MARKET_CRAFT_UPGRADE,170,150,0,0); // 射程再+1（需伐木加工）
        researchIfPossible(info,BUILDING_STOCK ,BUILDING_STOCK_UPGRADE_DEFENSE_ARCHER,100,0,0,0); // 护甲+2
    }
}

bool researchIfPossible(const tagInfo& info,int buildingType,int action,
         int needFood,int needWood,int needStone,int needGold)
{
            if(techDone.count(action)) return true;          // 已研发/在研发 → 不重复下令
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

    if(gameFrame - lastScanFrame >= 25){                // 敌人 25 格内 → 拉黑
        lastScanFrame = gameFrame;
        for(const tagArmy& e : info.enemy_armies){
            if(e.Blood <= 0) continue;
            double edr = e.DR / B, eur = e.UR / B;
            for(int i = 0; i < NP; i++)
                if(!netDone[i] && calDistance(netL[i], netU[i], edr, eur) < 25) netDone[i] = 1;
        }
    }
    if(scoutTgtDR >= 0 && netCur >= 0 && netCur < NP && netDone[netCur]){
        scoutTgtDR = -1; scoutTgtUR = -1;
        stillCnt = 0;
        std::cerr << "[net] 当前点被敌人拉黑，切下一个 (cur=" << netCur << ")" << std::endl;
    }
    double pdr = -1, pur = -1; bool got = false;
    for(const tagArmy& p : info.armies){
        if(p.SN != priestSN) continue;
        pdr = p.DR; pur = p.UR; got = true; break;
    }
    if(!got) return;

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

    const int step   = 5;        // ★ 5 格一个点（比 6 密，推进更快）
    const int margin = 3;

    const double A    = double(enemyDR - mycenterDR);
    const double Bv   = double(enemyUR - mycenterUR);
    const double K    = ((double)enemyDR*enemyDR + (double)enemyUR*enemyUR
                       - (double)mycenterDR*mycenterDR - (double)mycenterUR*mycenterUR) / 2.0;
    const double DLEN = sqrt(A*A + Bv*Bv);

    // ===== 一条龙：逐列收集 → 奇偶列反向 → 拼成一条连续路径 =====
    vector<double> tmpL, tmpU;
    int keptCol = 0;                                   // 只数"真的有保留点的列"
    for(int l = margin; l < MAP_L - margin; l += step){
        vector<int> us;
        for(int u = margin; u < MAP_U - margin; u += step){
            if(DLEN > 1e-9 && (A*l + Bv*u - K) > 0) continue;   // 只留我方半边
            us.push_back(u);
        }
        if(us.empty()) continue;
        if(keptCol % 2 == 1) reverse(us.begin(), us.end());      // ★ 蛇形：相邻两列首尾相接
        for(size_t i = 0; i < us.size(); i++){
            tmpL.push_back(double(l));
            tmpU.push_back(double(us[i]));
        }
        keptCol++;
    }
    if(tmpL.empty()) return;

    // 起点 = 离基地最近的那个点（旋转后"接缝"落在最远端，路径依旧连续）
    int start = 0; double bestD = 1e18;
    for(size_t i = 0; i < tmpL.size(); i++){
        double dd = (tmpL[i]-mycenterDR)*(tmpL[i]-mycenterDR)
                  + (tmpU[i]-mycenterUR)*(tmpU[i]-mycenterUR);
        if(dd < bestD){ bestD = dd; start = (int)i; }
    }
    for(size_t k = 0; k < tmpL.size(); k++){
        int i = (start + (int)k) % (int)tmpL.size();
        netL.push_back(tmpL[i]);
        netU.push_back(tmpU[i]);
    }
    netL.push_back(mycenterDR);
    netU.push_back(mycenterUR);

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
    if(techDone.count(BUILDING_RANGE_UPGRADE_COMPOSITE_BOW)&&info.armies.size()>15){
        
        for(const tagArmy& a : info.armies){
            if(a.SN == priestSN) continue;
            if(a.Sort == AT_COMPOSITE_BOWMAN || a.Sort == AT_STONE_THROWER) continue;
            HumanAction(a.SN, a.SN);                                     // 自毁
        }
    }
    if(!isArmyReady && nBow+nThrow >= wantBowman-10) isArmyReady = true;           // 弓兵成型 → 转入反攻
    // 科技：复合弓（180 食 + 100 木）
    if(nBow < wantBowman&&info.civilizationStage>=CIVILIZATION_BRONZEAGE)
        researchIfPossible(info, BUILDING_RANGE, BUILDING_RANGE_UPGRADE_COMPOSITE_BOW, 180, 100, 0, 0);
    bool researchingBow = false;
    for(const tagBuilding& b : info.buildings)
        if(b.Project == BUILDING_RANGE_UPGRADE_COMPOSITE_BOW) researchingBow = true;
    if(researchingBow) return;    
    if(!techDone.count(BUILDING_RANGE_UPGRADE_COMPOSITE_BOW)) return;
    homeTarget=13;
    // 造复合弓兵（40 食 + 20 金）
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
    if(info.enemy_armies.empty()&&siegeSN>=0){
        for(const tagArmy& p:info.armies){
            if(p.SN != priestSN) continue;
            if(p.ConvertCooldown>0) break;
            if(p.WorkObjectSN!=siegeSN){
                HumanAction(priestSN,siegeSN);
            }
            break;
        }
    }
    if(info.enemy_armies.empty()&&siegeSN>=0&&etowerNum==-1) return;
    for(const tagArmy& a:info.armies){
        if(a.SN == priestSN) continue;
        if(a.Sort != AT_COMPOSITE_BOWMAN && a.Sort !=AT_STONE_THROWER) continue;
        
        if(info.enemy_armies.empty()&&etowerNum==-1&&siegeSN==-1){
            HumanMove(a.SN,enemyDR*B,enemyUR*B);
            continue;
        }
        if(!info.enemy_armies.empty()){
            const double backR  = (a.Sort == AT_STONE_THROWER) ? 8.0 : 5.0;
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
           if(calDistance(e.DR, e.UR, hx, hy) < 35 * BLOCKSIDELENGTH){ threat = true; break; }
        }
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
                        break;
                    }
                }
            }
            if(attackTgt>=0){
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
               break;
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
                }
                break;
            }
        }
    }
}
