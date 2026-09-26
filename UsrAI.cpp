#include "UsrAI.h"
#include <iostream>
#include <cstdlib>
#include<algorithm>

using namespace std;
tagGame tagUsrGame;
ins UsrIns;
/*##########DO NOT MODIFY THE CODE ABOVE##########*/

#include <cmath>
void UsrAI::processData()
{   
//#################################################################################################
    tagInfo info = getInfo();  //本帧快照
    gameFrame = info.GameFrame;
    if(gameFrame%25!=0) return;



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
    manageAttackKite(info);

    // 防御
    manageDefense(info);

    // 升级时代
    manageUpdate(info);
}

int UsrAI::getPhase(int& gameFrame){
    if(gameFrame >= 25000) phase = PHASE_ATTACK;
    else if(gameFrame>=13500) phase=PHASE_DEFEND23;
    else if(gameFrame>=5800) phase=PHASE_DEFEND1;
    else phase=PHASE_ECON;
    return phase;
}

void UsrAI::manageInit(const tagInfo& info)
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



void UsrAI::manageProduction(const tagInfo& info)
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



void UsrAI::manageFarmer(const tagInfo& info)
{
    //人员分配
    int pop = (int)info.farmers.size();
    int xx = taskTarget[TASK_GOLD] = 2;
    int homenum = 0;
    int rangenum = 0;
    int towernum = 0;
    for(const tagBuilding& b: info.buildings){
        if(b.Type == BUILDING_HOME && b.Percent>=100) homenum++;
        if(b.Type == BUILDING_RANGE && b.Percent >= 100) rangenum++;
        if(b.Type == BUILDING_ARROWTOWER) towernum++;
    }
    if(rangenum>=rangeTarget) homeTarget = 13;
    
    if(homenum>=13&&rangenum>=rangeTarget) xx+= 0;
    else xx+= 1;

    if(towernum>=towerTarget) needTower=false;
    if(needTower&&(towernum*150+info.Stone<=450)) xx+=taskTarget[TASK_STONE] = 2;
    else xx+=taskTarget[TASK_STONE] = 0;

    if(info.civilizationStage<=CIVILIZATION_BRONZEAGE){
        xx+=taskTarget[TASK_WOOD] = (pop-xx)*40/100;
    }else{
        xx+=taskTarget[TASK_WOOD] = (pop-xx)*30/100;
    }

    taskTarget[TASK_FOOD] = (pop-xx);

    if(taskTarget[TASK_STONE] == 0 && !stoneSN.empty()){
        for(auto it = stoneSN.begin();it!=stoneSN.end();++it){
            freeSN[it->first]=it->second;
            stoneSN.erase(it);
        }
    }
    if(homenum>=13&&rangenum>=rangeTarget){
        freeSN[buildSN]=-1;
        buildSN=-1;
    }
    if(rangenum>=3){
        taskTarget[TASK_FARM] = taskTarget[TASK_FOOD];
        taskTarget[TASK_FOOD] = 0;
        for(auto it = foodSN.begin();it!=foodSN.end();++it){
            farmSN[it->first]=foodSN[it->second];
            foodSN.erase(it);
        }
    }

    // 检查死亡
    std::map<int,int> aliveSN;
    for(const tagFarmer& f : info.farmers){
        if(!aliveSN.count(f.SN)){
            aliveSN[f.SN]=1;
        }
    }
    for(auto it = humantask.begin();it!=humantask.end();++it){
        if(!aliveSN.count(it->first)){
            humantask.erase(it);
            if(humantaskType.count(it->first)){
                humantask.erase(it->first);
            }
        }
    }
    if(buildSN>=0&& !aliveSN.count(buildSN)) buildSN=-1;
  
    for(const tagFarmer& f: info.farmers){
        if(!humantask.count(f.SN)){
            humantask[f.SN]=-1;
            humantaskType[f.SN] = -1;
        }
    }
    // 检查超员
    int curnum[6]={0,0,0,0,0,0};
    for(auto it=humantask.begin();it!=humantask.end();++it){
        if(it->second==-1){
            freeSN[it->first]=it->second;
        }else{
            curnum[it ->second]++;
        }
    }
    for(int i=1;i<=5;++i){
        while(curnum[i] - taskTarget[i] > 0 ){
            for(auto it=humantask.begin();it!=humantask.end();++it){
                if(it->second==i){
                    it->second=-1;
                    curnum[i]--;
                    if(curnum[i]==taskTarget[i]) break;
                }
            }
            break;
        }
    }
    // 不足补齐
    for(int i=1;i<=5;++i){
        while(curnum[i]-taskTarget[i]<0){
            for(auto it=humantask.begin();it!=humantask.end();++it){
                if(it->second==-1){
                    it->second=i;
                    curnum[i]++;
                    if(curnum[i]==taskTarget[i]) break;
                }
            }
            break;
        }
    }

    // 执行任务
    for(const tagFarmer& f:info.farmers){
        // 去除不可下达任务情况
        if(!humantask.count(f.SN)) continue;
        if(f.SN==buildSN) continue;
        if(humantask[f.SN]==6){
            if(f.NowState!= HUMAN_STATE_IDLE){
                continue;
            }else{
                humantask[f.SN]=-1;
                humantaskType[f.SN]=-1;
                continue;
            }
        }
        if(humantaskType.count(f.SN)){
            if(humantaskType[f.SN]==humantask[f.SN]) continue;
        }
        if(f.NowState != HUMAN_STATE_IDLE) continue;
        if(humantask[f.SN]==1){
            int fSN = findNearestFood(info,f.DR,f.UR,takenSource);
            if(fSN>=0){
                takenSource[fSN]=1;
                humantaskType[f.SN]=1;
                HumanAction(f.SN,fSN);
            }
        }else if(humantask[f.SN]==5){
            if(info.Wood>=BUILD_FARM_WOOD && countBuilding(info,BUILDING_MARKET)>0){
                int bd = -1,bu = -1;
                if(findBuildSite(info,BUILDING_FARM,myfameDR,myfameUR,bd,bu)){
                    HumanBuild(f.SN,BUILDING_FARM,bd,bu);
                    humantaskType[f.SN] = 5;
                }
            }else{

            }
        }else{
            int resType = (humantask[f.SN] == TASK_WOOD) ? RESOURCE_TREE :
                          (humantask[f.SN] == TASK_STONE) ? RESOURCE_STONE :
                                               RESOURCE_GOLD;
            int rsn = findNearestResource(info,resType,f.DR,f.UR);
            if(rsn >= 0){
                takenSource[rsn]=1;
                humantaskType[f.SN]=humantask[f.SN];
                HumanAction(f.SN,rsn);
            }
        }
    }
}

int UsrAI::findNearestFood(const tagInfo& info,double dr,double ur,const std::map<int,int>& takenSource)
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

int UsrAI::findNearestResource(const tagInfo& info,int resType,double dr,double ur)
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

bool UsrAI::inEnemyZone(double dr, double ur)
{
    if(enemyDR < 0 || enemyUR < 0) return false;      // 还没定位敌人锚点 → 不限制
    double d = calDistance(dr, ur,
                           enemyDR * BLOCKSIDELENGTH,
                           enemyUR * BLOCKSIDELENGTH);
    return d <= enemySafeBlocks * BLOCKSIDELENGTH;
}

int UsrAI::countBuilding(const tagInfo& info,int type)
{
    int cnt = 0;
    for(const tagBuilding& b : info.buildings){
        if(b.Type == type && b.Percent >= 100) cnt++;
    }
    return cnt;
}

bool UsrAI::findBuildSite(const tagInfo& info, int buildingType,int myDR,int myUR, int& outDR, int& outUR)
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

int UsrAI::buildBuildingSize(int type)
{
    if(type == BUILDING_HOME || type == BUILDING_ARROWTOWER) return 2;
    return 3;
}

bool UsrAI::canBuildHere(const tagInfo& info,int dr,int ur)
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

bool UsrAI::overlapsBuilding(const tagInfo& info, int dr, int ur, int size)
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

bool UsrAI::unitOnSite(const tagInfo& info, int dr, int ur, int size)
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



void UsrAI::manageBuild(const tagInfo& info)
{
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
            if(f.SN ==buildSN && f.NowState == HUMAN_STATE_IDLE){
                if(foundationSN >= 0){
                   HumanAction(buildSN,foundationSN);
                }else{
                   HumanBuild(buildSN,wantType,buildDR,buildUR);
                }
                return;
            }
        }
    }
}

void UsrAI::manageResearch(const tagInfo& info){
    // ① 观察：任何"正在研发中"的动作都记下来（Project 就是当前动作）→ 之后不再重复下令
    for(const tagBuilding& b : info.buildings)
        if(b.Project != ACT_NULL) techDone.insert(b.Project);
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

bool UsrAI::researchIfPossible(const tagInfo& info,int buildingType,int action,
         int needFood,int needWood,int needStone,int needGold){
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

void UsrAI::manageScout(const tagInfo& info)
{
    if(phase!= PHASE_ECON) return;
    if(scoutState == SCOUT_DONE) return;
    const double B = BLOCKSIDELENGTH;

    if(priestSN<0){
        for(const tagArmy& a : info.armies){
            if(a.Sort == AT_PRIEST){ priestSN = a.SN;break;}
        }
        if(priestSN < 0) return; 
    }

    double hx = (myarrowDR >= 0 ? myarrowDR : mycenterDR) * B;
    double hy = (myarrowUR >= 0 ? myarrowUR : mycenterUR) * B;

    if(scoutState == SCOUT_WAIT){
        if(mycenterDR >= 0 && info.GameFrame >= 100){
            scoutState = SCOUT_OUT;
        }
        return;
    }

    if(scoutState == SCOUT_RETURN){
        double d;
        for(const tagArmy& p:info.armies){
            if(p.SN == priestSN){
                d=calDistance(p.DR,p.UR, hx, hy);
                break;
            }
        }
        if(d<3*B){
            if(phase!=PHASE_ECON){
                scoutState = SCOUT_DONE; 
            }else{
                homeRestUntil = gameFrame + 25;          // 在家歇~10秒
                scoutState = SCOUT_REST;
                scoutTgtDR = -1; scoutTgtUR = -1;
            }
        }else if(scoutTgtDR != hx || scoutTgtUR != hy){
            bool ff = true;
            for(const tagArmy& p:info.armies){
                if(p.SN == priestSN&&p.NowState!=HUMAN_STATE_WALKING&&gameFrame-lastOrderFrame>=25){
                    ff=false;
                    break;
                }
            }
            if(ff) return;
            scoutTgtDR = hx; scoutTgtUR = hy;
            lastOrderFrame = gameFrame;
            HumanMove(priestSN, hx, hy);
        }
        return;
    }

    if(scoutState == SCOUT_REST){
        if(gameFrame < homeRestUntil) return;
        if(!netReady) buildScoutNet();
        netCur = -1;
        scoutTgtDR = -1; scoutTgtUR = -1;
        scoutState = SCOUT_OUT;
        return;
    }

    if(!netReady) buildScoutNet();
    if(netL.empty()) return;

    if(gameFrame - lastScanFrame >= 25){
        lastScanFrame = info.GameFrame;
        for(const tagArmy& e : info.enemy_armies){
            if(e.Blood <= 0) continue;
            double edr = e.DR / B, eur = e.UR / B;
            for(size_t i = 0; i < netL.size(); i++)
                if(!netDone[i] && calDistance(netL[i], netU[i], edr, eur) < 25)
                    netDone[i] = 1;
        }
    }

    if(scoutTgtDR >= 0 && netCur >= 0 && netCur < (int)netL.size() && netDone[netCur]){
        scoutTgtDR = -1; scoutTgtUR = -1;
    }

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
        lastDR = scoutTgtDR;
        lastUR = scoutTgtUR;
        lastOrderFrame = gameFrame;
        return;
    }
    double dw;
    bool stopped;
    for(const tagArmy& p:info.armies){
        if(p.SN == priestSN){
            dw = calDistance(p.DR, p.UR, scoutTgtDR, scoutTgtUR);
            stopped = (fabs(p.DR-lastDR) < 0.5 && fabs(p.UR-lastUR) < 0.5);
            break;
        }
    }
    if(dw<3*B||stopped){
        scoutTgtDR = -1;
        scoutTgtUR = -1;
        return;
    }
    if(gameFrame-lastOrderFrame >=50){
        HumanMove(priestSN,scoutTgtDR,scoutTgtUR);
        lastOrderFrame = gameFrame;
        lastDR = scoutTgtDR;
        lastUR = scoutTgtUR;
    }
    
}


void UsrAI::buildScoutNet()
{
    netL.clear();
    netU.clear();
    netDone.clear();
    netCur=-1;
    netNext=0;
    if(mycenterDR<0||enemyDR<0) return;

    const int inset =10;
    const int step = 15;
    const int n = 6;
    const int sx = (mycenterDR<50)?1:-1;
    const int sy = (mycenterDR<50)?1:-1;
    
    // 分界线（两垒连线的中垂线）： A*l + Bv*u = K
    //   "我方这边" = A*l + Bv*u - K <= 0（取等号就是正好在对角线上）
    const double A  = double(enemyDR - mycenterDR);
    const double Bv = double(enemyUR - mycenterUR);
    const double K  = ((double)enemyDR*enemyDR + (double)enemyUR*enemyUR
                       - (double)mycenterDR*mycenterDR - (double)mycenterUR*mycenterUR) / 2.0;
    const double DLEN = sqrt(A*A + Bv*Bv);        
    const double LIM  = 6 * DLEN;              // 允许越过分界线 6 格
    
    for(int i = 0; i < n; i++){
        int l = (sx > 0) ? (inset + i*step) : (MAP_L - inset - i*step);
        for(int k = 0; k < n; k++){
            int row = (i % 2 == 0) ? k : (n - 1 - k);      // 蛇形：奇数列反向，避免横跳
            int u = (sy > 0) ? (inset + row*step) : (MAP_U - inset - row*step);
            if(A*l + Bv*u - K > LIM) continue;             // 敌方那半边超10格子 → 丢掉
            netL.push_back(double(l)); netU.push_back(double(u));
        }
    }

    if(fabs(Bv) > 1e-9){
        int lastL = -1000;
        for(int l = inset; l <= MAP_L - inset; l++){
            double uf = (K - A*l) / Bv;                    // 解出分界线上的 u
            int u = (int)(uf + (uf >= 0 ? 0.5 : -0.5));
            if(fabs(uf - double(u)) > 1e-6) continue;      // 不是整数格点 → 跳过
            if(u < inset || u > MAP_U - inset) continue;
            if(l - lastL < 9) continue;                    // 沿线间距
            lastL = l;
            netL.push_back(double(l));
            netU.push_back(double(u));
        }
    }
    netL.push_back(mycenterDR);                            // 最后回家，把这个环接上
    netU.push_back(mycenterUR);

    netDone.resize(netL.size(),0);
    netReady = true;
}

void UsrAI::manageArmyKite(const tagInfo &info)
{
    int nBow = 0, nThrow = 0;
    for(const tagArmy& a : info.armies){
        if(a.Sort == AT_COMPOSITE_BOWMAN)   nBow++;
        else if(a.Sort == AT_STONE_THROWER) nThrow++;
    }
    // 兵种清洗：留复合弓兵 / 投石车 / 祭司，其余自杀
    if(techDone.count(BUILDING_RANGE_UPGRADE_COMPOSITE_BOW)){
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
    if(!techDone.count(BUILDING_RANGE_UPGRADE_COMPOSITE_BOW)) return;
    int homenum = countBuilding(info, BUILDING_HOME);                    // 房子不够先别造
    if(homenum*4 <= nBow + nThrow+20) return;
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

void UsrAI::manageAttackKite(const tagInfo &info)
{
    if(!isArmyReady) return;
    const double B = BLOCKSIDELENGTH;
    int siegeSN = -1;
    int etowerSN = -1, etowerBL = -1, etowerBU = -1; double bestTd = 1e18;
    int etowerNum=-1;
    for(const tagBuilding& b : info.enemy_buildings){
        if(b.Type != BUILDING_SIEGE){
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
                  HumanAction(a.SN,towerSN);
        }
    }
}

void UsrAI::manageDefense(const tagInfo &info)
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
            if(towerSN<0) towerSN = b.SN;
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
            if(eSN >= 0 && eD <= PULL*B){
                double rx, ry;                                // ① 被贴死 → 撤回自家角
                rx = mycenterDR*B + B/2;
                ry = mycenterUR*B + B/2;
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
void UsrAI::manageUpdate(const tagInfo &info)
{ 
    if(info.civilizationStage < CIVILIZATION_BRONZEAGE && info.Meat >=810){
        for(const tagBuilding& b : info.buildings){
            if(b.Type == BUILDING_CENTER){
                if(b.Project == ACT_NULL){
                    //researchIfPossible(info,BUILDING_MARKET,BUILDING_MARKET_FARM_UPGRADE,200,50,0,0);
                    BuildingAction(b.SN,BUILDING_CENTER_UPGRADE);
                    homeTarget = 13;
                }
                break;
            }
        }
    }
}
