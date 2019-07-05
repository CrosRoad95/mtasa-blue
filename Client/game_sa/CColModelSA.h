/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        game_sa/CColModelSA.h
 *  PURPOSE:     Header file for collision model entity class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#pragma once

#include <windows.h>
#include <game/CColModel.h>

#define FUNC_CColModel_Constructor 0x40FB60
#define FUNC_CColModel_Destructor 0x40F700

typedef struct
{
    CVector vecMin;
    CVector vecMax;
    CVector vecOffset;
    float   fRadius;
} CBoundingBoxSA;

struct CColSphereSA
{
    float        fRadius;
    CVector      vecCenter;
    uchar        material;
    uchar        flags;
    uchar        lighting;
    uchar        light;

    CColSphereSA() {}
};

struct CColBoxSA
{
    CVector      min;
    CVector      max;
    uchar        material;
    uchar        flags;
    CColLighting lighting;
    uchar        light;

    CColBoxSA(){}
};

typedef struct
{
    unsigned short v1;
    unsigned short v2;
    unsigned short v3;
    EColSurface    material;
    CColLighting   lighting;
} CColTriangleSA;

typedef struct
{
    BYTE pad0[12];
} CColTrianglePlaneSA;

typedef struct
{
    char  version[4];
    DWORD size;
    char  name[0x18];
} ColModelFileHeader;

struct CompressedVector
{
    signed __int16 x;
    signed __int16 y;
    signed __int16 z;
    CVector        getVector() const { return CVector(x * 0.0078125f, y * 0.0078125f, z * 0.0078125f); }
    void           setVector(CVector vec)
    {
        x = static_cast<signed __int16>(vec.fX * 128);
        y = static_cast<signed __int16>(vec.fY * 128);
        z = static_cast<signed __int16>(vec.fZ * 128);
    }
    CompressedVector() {}
};

typedef struct
{
    WORD                 numColSpheres;
    WORD                 numColBoxes;
    WORD                 numColTriangles;
    BYTE                 ucNumWheels;
    BYTE                 pad3;
    CColSphereSA*        pColSpheres;
    CColBoxSA*           pColBoxes;
    void*                pSuspensionLines;
    CompressedVector*    pVertices;
    CColTriangleSA*      pColTriangles;
    CColTrianglePlaneSA* pColTrianglePlanes;

    ushort getNumVertices() const
    {
        ushort count = 0;
        CColTriangleSA triangle;
        for (uint i = 0; numColTriangles > i; i++)
        {
            triangle = pColTriangles[i];
            count = std::max(count, std::max(triangle.v1, std::max(triangle.v2, triangle.v3)));
        }
        return count + 1;
    }
} CColDataSA;

class CColModelSAInterface
{
public:
    CBoundingBoxSA boundingBox;
    BYTE           level;
    BYTE           unknownFlags;
    BYTE           pad[2];
    CColDataSA*    pColData;
};

class CColModelSA : public CColModel
{
public:
    CColModelSA();
    CColModelSA(CColModelSAInterface* pInterface);
    ~CColModelSA();

    CColModelSAInterface* GetInterface() { return m_pInterface; }
    void                  Destroy() { delete this; }

private:
    CColModelSAInterface* m_pInterface;
    bool                  m_bDestroyInterface;
};
