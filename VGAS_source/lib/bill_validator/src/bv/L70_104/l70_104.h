#ifndef _L70_104V_H_
#define _L70_104V_H_
#include "BillValidator.h"
#include "validatorHelper.h"

class L70_104 : public BillValidatorBaseClass
{
private:
    ICT104_BUS *bus;
    void processUType();
    void processVType();

public:
    L70_104(ICT104_BUS *bus);
    virtual ~L70_104();
    void loop();
};
#endif