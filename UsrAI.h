#ifndef USRAI_H
#define USRAI_H

#include "ai.h"
#include "Army.h"
#include <unordered_map>
#include <map>
#include <vector>

extern tagGame tagUsrGame;
extern ins UsrIns;

/*##########DO NOT MODIFY THE CODE ABOVE##########*/

#include <set>

class UsrAI:public AI
{
public:
    UsrAI(){this->id=0;}
    ~UsrAI(){}

private:
    void processData() override;
    tagInfo getInfo(){return tagUsrGame.getInfo();}
    int AddToIns(instruction ins) override
    {
        UsrIns.lock.lock();
        ins.id=UsrIns.g_id;
    
        UsrIns.g_id++;
        UsrIns.instructions.push(ins);
        UsrIns.lock.unlock();
        return ins.id;
    }
    void clearInsRet() override
    {
        tagUsrGame.clearInsRet();
    }
    /*##########DO NOT MODIFY THE CODE IN THE CLASS##########*/
    
    int gameFrame=-1;

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
    std::map<int,int> humantask;
    std::map<int,int> humantaskType;
    std::map<int,int> farmerSN;
    std::map<int,int> foodSN;
    std::map<int,int> woodSN;
    std::map<int,int> stoneSN;
    std::map<int,int> goldSN;
    std::map<int,int> farmSN;
    std::map<int,int> freeSN;
    std::map<int,int> takenSource;
    int buildSN=-1;
    enum FarmerTask{T0,TASK_FOOD,TASK_WOOD,TASK_STONE,TASK_GOLD,TASK_FARM,TASK_BUILD};
    int taskTarget[7] = {0,0,0,0,0,0,0};
    double enemySafeBlocks = 20.0;

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
    enum GamePhase{ PHASE_ECON, PHASE_DEFEND1, PHASE_DEFEND23, PHASE_ATTACK };
    enum ScoutState{ SCOUT_OUT, SCOUT_RETURN, SCOUT_REST, SCOUT_DONE };  // 加 REST
    int phase = PHASE_ECON;
    int scoutState;
    int priestSN = -1;
    double lastDR=-1.0,lastUR=-1.0;
    double scoutTgtDR = -1;
    double scoutTgtUR = -1;
    int lastOrderFrame = 0;
    int homeRestUntil = 0;
    double lastPDR = -1, lastPUR = -1;   // 上次采样时祭司所在位置（判断有没有真的在挪）
    int    stuckCnt = 0;                 // 连续多少次采样位置没变
    int    pointStartFrame = 0;          // 当前网点是从哪一帧开始追的
    
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
};

/*##########YOUR CODE BEGINS HERE##########*/




/*##########YOUR CODE ENDS HERE##########*/
#endif // USRAI_H