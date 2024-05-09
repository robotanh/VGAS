#ifndef _L70_H_
#define _L70_H_
#include "BillValidator.h"
#include "validatorHelper.h"
#include "protocol/PULSE/ICTPULSE.h"
class L70 : public BillValidatorBaseClass
{
private:
    ICTPULSE_BUS *bus;
public:
    L70(ICTPULSE_BUS *bus);
    virtual ~L70();
    void loop();
};
#endif