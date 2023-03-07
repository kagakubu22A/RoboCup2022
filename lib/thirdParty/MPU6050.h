#ifndef MPU6050_H
#define MPU6050_H

#include <Arduino.h>
#include <Wire.h>

#include "i2c_ReadWrite.h"

#define X 0
#define Y 1
#define Z 2
#define M_S2 0
#define M_S 1
#define M 2
#define DEG_S 0
#define DEG 1

#define ACCEL 0
#define GYRO 1

#define ACCEL_X 0
#define ACCEL_Y 1
#define ACCEL_Z 2
#define GYRO_X 3
#define GYRO_Y 4
#define GYRO_Z 5

#ifndef PI
#define PI 3.141592653
#endif

#define G 9.80665

class MPU6050
{
public:
    // private:
    TwoWire *_wire = &Wire;
    uint8_t _filterStrength;                                           //フィルターに用いるデータ数(1~255)
    uint8_t _initResister[5] = {0x1A, 0x1B, 0x1C, 0x37, 0x6B};         // initで書き込むレジスタ
    uint8_t _initValue[5] = {0x05, 0x00, 0x00, 0x02, 0x00};            // initでセンサに書き込む値
    uint8_t _resisterToRead[6] = {0x3B, 0x3D, 0x3F, 0x43, 0x45, 0x47}; //読み込むレジスタ
    double _resolution[2] = {0.00006105565, 0.00763195652};            //分解能(既定は加速度+-2G、ジャイロ+-250°/s)
    double _deadzone[2] = {0.0, 0.0};                                  //絶対値がこれより小さいと出力値は0.0
    int16_t _raw[6];                                                   //センサから取得した生値
    int32_t _valueDifferenceSum[6];                                    //センサの誤差の和
    int16_t _valueDifference[6];                                       //センサの誤差
    int16_t _filterStorage[6][256];                                    //フィルタ用データ格納庫
    uint8_t _filterNumber;                                             //更新するデータの場所
    int32_t _filteredValueSum[6];                                      //フィルタの和
    int16_t _filteredValue[6];                                         //フィルタ補正後の値(単位無)
    bool _isIntegrating = true;                                        //積分のon/off
    double _integralTime;                                              //積分時間の差分
    double _timeDifference;                                            //前回処理時の経過時間

    void _getData();   //センサから生値を得る
    void _adjust();    //誤差を測定
    void _filter();    //フィルタ補正(移動平均フィルタ)
    void _convert();   //データを単位有に変換
    void _integrate(); //積分
    void _reset();     //センサの初期化
    void _clear(); //値をクリアする(リセットはしない)

    // public:
    i2c_ReadWrite i2c = i2c_ReadWrite(); // i2c読み書き用クラス
    uint8_t address = 0x68;              //加速度・ジャイロセンサのアドレス
    double accel[3][3];                  //加速度センサの値[軸][次元]
    double gyro[3][2];                   //ジャイロセンサの値[軸][次元]
    double polar[3];                     //加速度ベクトルの極座標(大きさ・xy平面角度・z軸方向傾き) 経線・緯線のイメージ
    bool resetRequest = 0;               //真の時read関数内で初期化

    //2023/01/18 add
    //リセット中かどうか(リセット中に読むのを防ぐ)
    bool resetingFlag=false;

    MPU6050(uint8_t filterStrength = 1, bool addressSelect = false); //コンストラクタ
    void changeResolution(bool sensorType, uint16_t resolution);     //分解能を変更(既定は加速度+-2G、ジャイロ+-250°/s)
    void changeDeadzone(bool sensorType, double deadzone);           //デッドゾーンを変更(既定は0.0)
    void changeFilterStrength(uint8_t filterStrength);               //フィルター強度を変更(既定は1)
    void switchIntegration(bool isIntegrating = true);               //積分のon/off
    void init(TwoWire *wire);                                        //センサの設定
    void read();                                                     //データを更新
};

#endif