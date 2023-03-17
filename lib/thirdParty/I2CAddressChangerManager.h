#ifndef __I2C_ADDRESS_CHANNGER_MANAGER_H__
#define __I2C_ADDRESS_CHANNGER_MANAGER_H__

#define PCA_ADDRESS 0x77

class I2CAddressChangerManager{
    private:
    I2CAddressChangerManager();
    private:

    public:
    static void ChangeAddress(unsigned char channel);

    static bool TakeSemaphoreAndChangeAddress(unsigned char channel, int waitms);

    static void ReleaseSemaphore();
};
#endif