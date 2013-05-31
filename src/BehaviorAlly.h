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
        virtual void checkMoveStateStance();
        virtual void checkMoveStateMove();
};

#endif // BEHAVIORALLY_H
