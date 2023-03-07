#ifndef I2C_READWRITE
#define I2C_READWRITE

#define REVERSE 0
#define REGULER 1

#include <Wire.h>

class i2c_ReadWrite
{
private:
    TwoWire *_wire = &Wire;

public:
    uint8_t data[256]; //読み取ったデータの格納庫

    i2c_ReadWrite();
    void init(TwoWire *wire);                                     //コンストラクタ
    bool exist(uint8_t addr);                                     //デバイスが検知されているか
    void waitUntilExist(uint8_t addr);                                        //デバイスが検知されるまで待つ
    void scan();                                                  //スレーブアドレス検出
    uint8_t write(uint8_t addr, uint8_t val);                     //スレーブに書き込み
    uint8_t write(uint8_t addr, uint8_t reg, uint8_t val);        //スレーブに書き込み
    bool read(uint8_t addr, uint8_t reg, uint8_t num);            //スレーブから読み込み
    uint8_t readU8(uint8_t addr, uint8_t reg, bool ord = true);   //スレーブから符号なし8ビットで読み込み
    uint16_t readU16(uint8_t addr, uint8_t reg, bool ord = true); //スレーブから符号なし16ビットで読み込み
    uint32_t readU32(uint8_t addr, uint8_t reg, bool ord = true); //スレーブから符号なし32ビットで読み込み
    uint64_t readU64(uint8_t addr, uint8_t reg, bool ord = true); //スレーブから符号なし64ビットで読み込み
    int8_t readS8(uint8_t addr, uint8_t reg, bool ord = true);    //スレーブから符号あり8ビットで読み込み
    int16_t readS16(uint8_t addr, uint8_t reg, bool ord = true);  //スレーブから符号あり16ビットで読み込み
    int32_t readS32(uint8_t addr, uint8_t reg, bool ord = true);  //スレーブから符号あり32ビットで読み込み
    int64_t readS64(uint8_t addr, uint8_t reg, bool ord = true);  //スレーブから符号あり64ビットで読み込み
};

#endif