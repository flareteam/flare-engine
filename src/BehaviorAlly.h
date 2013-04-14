#ifndef BEHAVIORALLY_H
#define BEHAVIORALLY_H

#include "BehaviorStandard.h"


class BehaviorAlly : public BehaviorStandard
{
    public:
        BehaviorAlly(Enemy *_e, EnemyManager *_em);
        virtual ~BehaviorAlly();
    protected:
    private:
        virtual void findTarget();
        bool is_facing(int x, int y, char direction, int x2, int y2);
        virtual void checkMoveStateStance();
        virtual void checkMoveStateMove();
};

#endif // BEHAVIORALLY_H
