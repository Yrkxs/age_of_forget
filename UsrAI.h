#ifndef USRAI_H
#define USRAI_H

#include "ai.h"
#include "Army.h"
#include <unordered_map>

extern tagGame tagUsrGame;
extern ins UsrIns;
/*##########DO NOT MODIFY THE CODE ABOVE##########*/

#include <set>

// ===== 定长容器：替代 std::set / std::map，零动态分配 =====
// 万一内存被写坏，长度非法就在每次访问前夹回 0 —— 不会像 std::set 那样
// 在析构时按垃圾指针遍历红黑树而直接崩溃。
static const int CAP_SET = 320;
struct FixSet {
    mutable int n = 0; int v[CAP_SET];
    void fix() const { if(n < 0 || n > CAP_SET) n = 0; }     // ★ 自愈
    void clear(){ n = 0; }
    int  size() const { fix(); return n; }
    bool empty() const { fix(); return n == 0; }
    bool has(int x) const { fix(); for(int i = 0; i < n; i++) if(v[i] == x) return true; return false; }
    void add(int x){ fix(); if(n < CAP_SET && !has(x)) v[n++] = x; }
};
struct FixCount {                                            // 替代 std::map<int,int>
    mutable int n = 0; int k[CAP_SET], c[CAP_SET];
    void fix() const { if(n < 0 || n > CAP_SET) n = 0; }
    int  size() const { fix(); return n; }
    int  keyAt(int i) const { return k[i]; }
    int  get(int key) const { fix(); for(int i = 0; i < n; i++) if(k[i] == key) return c[i]; return 0; }
    void inc(int key){ fix(); for(int i = 0; i < n; i++) if(k[i] == key){ c[i]++; return; }
                       if(n < CAP_SET){ k[n] = key; c[n] = 1; n++; } }
    void dec(int key){ fix(); for(int i = 0; i < n; i++) if(k[i] == key){ if(c[i] > 0) c[i]--; return; } }
};
// 替代 std::map<int,V>：定长、零动态分配；只实现我们用到的 [] / find / end / 范围 for
template <typename V>
struct FixMap {
    struct Ent { int first; V second; };
    mutable int n = 0;
    Ent e[CAP_SET];
    void fix() const { if(n < 0 || n > CAP_SET) n = 0; }
    struct iterator {
        Ent* p;
        Ent* operator->() const { return p; }
        Ent& operator*()  const { return *p; }
        bool operator==(const iterator& o) const { return p == o.p; }
        bool operator!=(const iterator& o) const { return p != o.p; }
        iterator& operator++(){ p++; return *this; }
    };
    V& operator[](int key){
        fix();
        for(int i = 0; i < n; i++) if(e[i].first == key) return e[i].second;
        if(n < CAP_SET){ e[n].first = key; e[n].second = V(); n++; }
        return e[n > 0 ? n - 1 : 0].second;
    }
    iterator find(int key) const {
        fix();
        for(int i = 0; i < n; i++) if(e[i].first == key) return iterator{ (Ent*)&e[i] };
        return iterator{ (Ent*)&e[n] };              // 等于 end()
    }
    iterator end()   const { fix(); return iterator{ (Ent*)&e[n] }; }
    iterator begin() const { fix(); return iterator{ (Ent*)&e[0] }; }
};
// 替代 std::vector<int>：定长、零动态分配；只实现我们用到的接口
struct FixVec {
    mutable int n = 0;
    int v[CAP_SET];
    void fix() const { if(n < 0 || n > CAP_SET) n = 0; }     // ★ 自愈
    struct iterator {
        int* p;
        int& operator*() const { return *p; }
        iterator& operator++(){ p++; return *this; }
        iterator  operator+(int k) const { return iterator{ p + k }; }
        bool operator==(const iterator& o) const { return p == o.p; }
        bool operator!=(const iterator& o) const { return p != o.p; }
    };
    size_t size()  const { fix(); return (size_t)n; }
    bool   empty() const { fix(); return n == 0; }
    void   clear(){ n = 0; }
    void   push_back(int x){ fix(); if(n < CAP_SET) v[n++] = x; }
    void   assign(size_t cnt, int val){ fix(); n = 0; for(size_t i = 0; i < cnt && n < CAP_SET; i++) v[n++] = val; }
    int&       operator[](int i){ fix(); return v[(i >= 0 && i < n) ? i : 0]; }
    const int& operator[](int i) const { fix(); return v[(i >= 0 && i < n) ? i : 0]; }
    iterator begin() const { return iterator{ (int*)&v[0] }; }
    iterator end()   const { fix(); return iterator{ (int*)&v[n] }; }
    iterator erase(iterator it){
        fix();
        int idx = (int)(it.p - v);
        if(idx < 0 || idx >= n) return end();
        for(int i = idx; i + 1 < n; i++) v[i] = v[i + 1];
        n--;
        return iterator{ (int*)&v[idx] };
    }
};

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

//################################################################################################################################

//######### 阶段系统 #########################################################
    enum GamePhase{ PHASE_ECON, PHASE_DEFEND1, PHASE_DEFEND23, PHASE_ATTACK };
    int phase = PHASE_ECON;
    int getPhase(const tagInfo& info);
    int frameCount = 0;

//######### 我方、敌方锚点 #########################
    int mycenterDR = -1,mycenterUR = -1;
    int enemyDR = -1,enemyUR = -1;
    int myarrowDR = -1,myarrowUR = -1;
    int myfameDR = -1,myfameUR = -1;
    
// ########## 建筑系统 #########################
    void manageBuild(const tagInfo& info);
    int marketTarget = 1;   // 市场目标数量
    int towerTarget = 4;
    int lastBuildDR = -1, lastBuildUR = -1;  // 上次建造的坐标
    int lastFailDR =-9999, lastFailUR = -9999;  // 上次建造失败的坐标
    bool arrowTechOrdered = false;  // 是否已研发箭科技
    int lastBuildOrder = -1;
    int homeTarget = 6,campTarget = 1;
    int collageTarget = 0;
    int rangeTarget = 3;
    int stableTarget = 0;
    double depotRangeBlocks = 20.0; // 建造仓库、谷仓最大距离
    bool needTower=true;


    bool unitOnSite(const tagInfo& info,int dr,int ur,int size);  // 判断是否有单位在给定点范围内（包含边界）

    bool canBuildHere(const tagInfo& info,int dr,int ur);  // 判断是否可以在给定点建建筑
    int countBuilding(const tagInfo& info,int type);  // 统计我方已建建筑数量
    int buildBuildingSize(int type);  // 根据建筑类型返回建筑大小
    bool overlapsBuilding(const tagInfo& info, int dr, int ur, int size);  // 判断是否与已有建筑重叠
    bool findBuildSite(const tagInfo& info, int buildingType,int myDR,int myUR, int& outDR, int& outUR);  // 建筑偏好方向
    
    double nearestDepotDist(const tagInfo& info,int dr, int ur, int type1,int type2);  // 两建筑物距离
    bool findNearBuildSite(const tagInfo& info, int buildingType, int cdr, int cur,
                              int maxR, int& outDR, int& outUR); // 资源点周围建存储建筑
// ########## 资源系统 #########################
    int findNearestResource(const tagInfo& info,int resType,double dr,double ur);  //给一个资源类型，找到离给定点最近的资源，返回SN
    int findNearestFood(const tagInfo& info, double dr, double ur,const FixSet& takenSource);  //给一个坐标，找到离给定点最近的食物资源，返回SN
    int findNearestAliveAnimal(const tagInfo& info, double dr, double ur);  //给一个坐标，找到离给定点最近的活动物，返回SN
    int findNearestFreeFarm(const tagInfo& info, double dr, double ur,const FixSet& takenFarm);  // 找最近没人种的已建成农田
//######## 村民任务系统 #########################
    void manageProduction(const tagInfo& info);
    int farmerTarget = 20;
    enum FarmerTask{TASK_FOOD,TASK_WOOD,TASK_STONE,TASK_GOLD,TASK_BUILD,TASK_FARM};
    FixVec taskList[6];     // 食物、木、石、金、建造、农场
    int taskTarget[6] = {0,0,0,0,0,0};  // 各任务目标人数，按人口算
    int chooseTask(int cur[6]);   // 选缺人最多的任务

    double enemySafeBlocks = 15.0;          // 村民禁区半径(格)：敌方锚点方圆
    int lastDepotFrame = -1000;
    void buildDepotIfNeeded(const tagInfo& info);
    
    bool inEnemyZone(double dr, double ur); // 判断某点是否在禁区内（输入细节坐标）

    void manageFarmer(const tagInfo& info);  // 任务表维护，指派任务
    void executeFarmerTasks(const tagInfo& info);   // 按照任务采集

// ########## 科技研发 #########################
    //enum ResearchType{}；
    int lastBowTechOrder = -1;
    FixSet techDone;    // 已研发/正在研发的科技动作，防止重复下令（定长，零动态分配）
    void manageResearch(const tagInfo& info);
    
    bool researchIfPossible(const tagInfo& info, int buildingType, int action,
                            int needFood, int needWood,int needStone,int needGold);

// ######### 探路系统 #########################
   enum ScoutState{ SCOUT_WAIT, SCOUT_OUT, SCOUT_RETURN, SCOUT_REST, SCOUT_DONE };  // 加 REST
   int homeRestUntil = 0;        // 在家休整到该帧
   int scoutState = SCOUT_WAIT;
    int priestSN = -1;                    // 祭司SN
    double scoutTgtDR = -1, scoutTgtUR = -1;   // 上次下令的点（防重复下令）
    int convertTarget = -1;               // 正在转化的敌人SN
    int lastOrderFrame = 0;      // 上次下移动令的帧

    // 路点网：只覆盖"离家更近那半边"；每个点只处理一次，到达/走不到/有敌人 → 永久拉黑
    int netCur = -1;                 // 当前正在去的网点（-1=没有目标）
    int netNext = 0;                 // 下一个待处理网点的搜索起点（单调前进）
    int wpOrderFrame = 0;            // 当前网点下令的帧（超时判断）
    int lastScanFrame = -1000;       // 上次扫敌人的帧
    bool netReady = false;           // 路点网是否已生成
    FixVec netL, netU;     // 路点网（格坐标）
    FixVec netDone;   // 1 = 已处理/永久拉黑
    void buildScoutNet();            // 生成路点网
    void manageScout(const tagInfo& info);
// 防御\反攻####################################################
    int haveStoneThrower = -1;
    int wantHoplite = 0;        // 目标方阵兵
    int wantCavalry = 26;        // 目标骑兵
    int nHop = 0,nCav = 0;
    bool isArmyReady = false; // 反攻
    int focusThrowerSN = -1;      // 全局锁定的投石车SN（只由 manageAttack 读写）
    int focusLostFrames = 0;      // 连续多少帧确认不到它
    int wantBowman = 30;      // 目标复合弓兵数量
    double bowRangeBlocks = 7.0;        // 复合弓兵当前真实射程（格），随科技动态更新
    int    bowRangeFrame  = -1000;      // 上次采样帧
    void   updateBowRange(const tagInfo& info);
    void manageAttackKite(const tagInfo& info);   // 风筝流进攻
    void manageArmyKite(const tagInfo& info);     // 只造复合弓兵 + 兵种清洗
    void manageDefense(const tagInfo& info); 
    // void manageArmy(const tagInfo& info); // 造兵
    // void manageAttack(const tagInfo& info); // 反攻与祭司
// ########## 打印系统 #########################
    // QString pendingMsg;   // 已删除：不再需要（AI 线程里少一个会被写坏的 Qt 对象）
//######### 指令分流（新增） ################################################
    // 引擎每收到一条新指令都会先 suspendRelation 取消这个单位手上的旧行动
    // （Core_List.cpp:159），所以“每帧重下同一条令”= 让单位永远从头开始。
    // 下面这些同名函数会遮蔽 AI:: 的版本，所有调用点自动过这一层，调用点一行都不用改。
    int  gameFrame = 0;                  // 当前游戏帧（单调，不受 frameCount 归零影响）
    int  ordersThisFrame = 0;            // 本帧已下指令数
    int  ordTotal = 0, ordSuppressed = 0, lastOrdReportFrame = 0;
    int  ordFarmer = 0, ordArmy = 0, ordBuild = 0;   // 诊断：分类计数
    int  ordN = 0;
    int  ordSN[CAP_SET], ordFrame[CAP_SET];
    long long ordKey[CAP_SET];
    static long long ordKeyOf(int type,int obSN,int option,int blockDR,int blockUR,double dr,double ur);
    static int  ordGapOf(int sn);
    bool ordAllow(int sn, long long key);
    int  HumanAction(int SN, int obSN);                          // 遮蔽 AI::HumanAction
    int  HumanMove(int SN, double DR0, double UR0);              // 遮蔽 AI::HumanMove
    int  HumanBuild(int SN, int BuildingNum, int blockDR, int blockUR);
    int  BuildingAction(int SN, int Action);
    int  OrderCancel(int sn);                                    // 取消某单位当前行动
};

/*##########YOUR CODE BEGINS HERE##########*/




/*##########YOUR CODE ENDS HERE##########*/
#endif // USRAI_H
