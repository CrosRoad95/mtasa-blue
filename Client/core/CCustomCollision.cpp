/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        core/CCustomCollision.cpp
 *  PURPOSE:     Per element custom collision class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#include "StdInc.h"

#include "../game_sa/CModelInfoSA.h"
#include "../game_sa/CEntitySA.h"

CCustomCollision::CCustomCollision()
{

}

CCustomCollision::~CCustomCollision()
{

}

CColModelSAInterface* CCustomCollision::GetOriginalColModel(CEntitySAInterface* pEntity)
{
    CModelInfoSA*         pModelInfoSA = (CModelInfoSA*)(g_pCore->GetGame()->GetModelInfo(pEntity->m_nModelIndex));
    CColModelSAInterface* pColModelInterface = pModelInfoSA->GetInterface()->pColModel;
    return pColModelInterface;
}

void CCustomCollision::ScaleCollision(CColModelSAInterface* pColModel, const CVector& scale)
{
    pColModel->boundingBox.fRadius *= std::max(scale.fX, std::max(scale.fY, scale.fZ)) * 2;
    pColModel->boundingBox.vecCenter *= (scale / 2);
    pColModel->boundingBox.vecMax *= scale;
    pColModel->boundingBox.vecMin *= scale;

    for (int i = 0; i < pColModel->pColData->numColSpheres; i++)
    {
        pColModel->pColData->pColSpheres[i].fRadius *= scale.fX;
        pColModel->pColData->pColSpheres[i].vecCenter *= scale;
    }

    for (int i = 0; i < pColModel->pColData->numColBoxes; i++)
    {
        pColModel->pColData->pColBoxes[i].max *= scale;
        pColModel->pColData->pColBoxes[i].min *= scale;
    }
    for (int i = 0; i < pColModel->pColData->getNumVertices(); i++)
    {
        pColModel->pColData->pVertices[i].x *= scale.fX;
        pColModel->pColData->pVertices[i].y *= scale.fY;
        pColModel->pColData->pVertices[i].z *= scale.fZ;
    }
}

CColModelSAInterface* CCustomCollision::CopyColModel(CColModelSAInterface* pColModel)
{
    CColModelSAInterface* pInterface = new CColModelSAInterface();
    pInterface->boundingBox = pColModel->boundingBox;
    pInterface->level = pColModel->level;
    memcpy(pInterface->pad, pColModel->pad, sizeof(BYTE) * 2);
    pInterface->unknownFlags = pColModel->unknownFlags;
    pInterface->pColData = new CColDataSA();
    pInterface->pColData->numColSpheres = pColModel->pColData->numColSpheres;
    pInterface->pColData->numColBoxes = pColModel->pColData->numColBoxes;
    pInterface->pColData->numColTriangles = pColModel->pColData->numColTriangles;
    pInterface->pColData->numColLines = pColModel->pColData->numColLines;
    pInterface->pColData->numShadowTriangles = pColModel->pColData->numShadowTriangles;
    pInterface->pColData->numShadowVertices = pColModel->pColData->numShadowVertices;
    pInterface->pColData->flags = pColModel->pColData->flags;

    if (pInterface->pColData->numShadowTriangles > 0)
    {
        pInterface->pColData->pShadowVertices = new CompressedVector[pInterface->pColData->numShadowTriangles];
        memcpy(pInterface->pColData->pShadowVertices, pInterface->pColData->pShadowVertices,
               sizeof(CompressedVector) * pInterface->pColData->numShadowTriangles);
    }

    if (pInterface->pColData->numShadowTriangles > 0)
    {
        pInterface->pColData->pShadowTriangles = new CColTriangleSA[pInterface->pColData->numShadowTriangles];
        memcpy(pInterface->pColData->pShadowTriangles, pColModel->pColData->pShadowTriangles,
               sizeof(CColTriangleSA) * pInterface->pColData->numShadowTriangles);
    }

    if (pInterface->pColData->numColSpheres > 0)
    {
        pInterface->pColData->pColSpheres = new CColSphereSA[pInterface->pColData->numColSpheres];
        memcpy(pInterface->pColData->pColSpheres, pColModel->pColData->pColSpheres, sizeof(CColSphereSA) * pInterface->pColData->numColSpheres);
    }

    if (pInterface->pColData->numColBoxes > 0)
    {
        pInterface->pColData->pColBoxes = new CColBoxSA[pInterface->pColData->numColBoxes];
        memcpy(pInterface->pColData->pColBoxes, pColModel->pColData->pColBoxes, sizeof(CColBoxSA) * pInterface->pColData->numColBoxes);
    }

    if (pInterface->pColData->numColTriangles > 0)
    {
        pInterface->pColData->pColTriangles = new CColTriangleSA[pInterface->pColData->numColTriangles];
        memcpy(pInterface->pColData->pColTriangles, pColModel->pColData->pColTriangles,
               sizeof(CColTriangleSA) * pInterface->pColData->numColTriangles);
    }

    //if (pInterface->pColData->numColTriangles > 0)
    //{
    //    pInterface->pColData->pColTrianglePlanes = new CColTrianglePlaneSA[pInterface->pColData->numColTriangles];
    //    memcpy(pInterface->pColData->pColTrianglePlanes, pColModel->pColData->pColTrianglePlanes,
    //           sizeof(CColTrianglePlaneSA) * pInterface->pColData->numColTriangles);
    //}
    pInterface->pColData->pColTrianglePlanes = nullptr;

    if (pInterface->pColData->numColLines > 0)
    {
        pInterface->pColData->pSuspensionLines = new CColLineSA[pInterface->pColData->numColLines];
        memcpy(pInterface->pColData->pSuspensionLines, pColModel->pColData->pSuspensionLines, sizeof(CColLineSA) * pInterface->pColData->numColLines);
    }

    ushort numVertices = pColModel->pColData->getNumVertices();
    if (numVertices > 0)
    {
        pInterface->pColData->pVertices = new CompressedVector[numVertices];
        memcpy(pInterface->pColData->pVertices, pColModel->pColData->pVertices, sizeof(CompressedVector) * numVertices);
    }
    return pInterface;
}

bool CCustomCollision::SaveOriginalCollision(WORD model)
{
    if (m_mapOriginalCollisions.find(model) == m_mapOriginalCollisions.end())
    {
        CModelInfoSA*         pModelInfoSA = (CModelInfoSA*)(g_pCore->GetGame()->GetModelInfo(model));
        CColModelSAInterface* pColModelInterface = pModelInfoSA->GetInterface()->pColModel;
        CColModelSAInterface* pCopiedColModel = CopyColModel(pColModelInterface);

        m_mapOriginalCollisions.insert({model, pCopiedColModel});
        return true;
    }
    return false;
}


void CCustomCollision::DrawCollisionDebug(CObject* pObject)
{
    CColModelSAInterface* colModel = GetCurrentCollision(pObject);
    CColDataSA*           pColData = colModel->pColData;

    if (pColData == nullptr)
        return;

    CVector vecPosition = *pObject->GetPosition();

    CVector      bboxMin = colModel->boundingBox.vecMin;
    CVector      bboxMax = colModel->boundingBox.vecMax;
    unsigned int color = (unsigned int)colModel;            // make color depends on collision
    color |= 0x000000ff;                                    // with alpha 255

    // g_pCore->GetGraphics()->DrawLine3DQueued(vecPosition - bboxMin, vecPosition + bboxMax, 10, (unsigned int)colModel,false);

    float fLineWidth = 4.f + pow(colModel->boundingBox.fRadius, 0.5f);

    CVector       trianglePosition[3];
    CVector       posMin;
    CVector       posMax;
    CColBoxSA*    pColBox;
    CColSphereSA* pColSphere;
    CVector       spherePos;

    for (int i = 0; i < pColData->numColTriangles; i++)
    {
        CColTriangleSA* pColTriangle = &pColData->pColTriangles[i];
        trianglePosition[0] = pColData->pVertices[pColTriangle->v1].getVector() + vecPosition;
        trianglePosition[1] = pColData->pVertices[pColTriangle->v2].getVector() + vecPosition;
        trianglePosition[2] = pColData->pVertices[pColTriangle->v3].getVector() + vecPosition;
        g_pCore->GetGraphics()->DrawLine3DQueued(trianglePosition[0], trianglePosition[1], fLineWidth, color, false);
        g_pCore->GetGraphics()->DrawLine3DQueued(trianglePosition[1], trianglePosition[2], fLineWidth, color, false);
        g_pCore->GetGraphics()->DrawLine3DQueued(trianglePosition[2], trianglePosition[0], fLineWidth, color, false);
    }

    for (int i = 0; i < pColData->numColBoxes; i++)
    {
        pColBox = &pColData->pColBoxes[i];
        posMin = pColBox->min + vecPosition;
        posMax = pColBox->max + vecPosition;

        // vertical lines
        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMin.fZ), CVector(posMin.fX, posMin.fY, posMax.fZ), fLineWidth, color, false);
        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMin.fY, posMin.fZ), CVector(posMax.fX, posMin.fY, posMax.fZ), fLineWidth, color, false);
        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMin.fZ), CVector(posMax.fX, posMax.fY, posMax.fZ), fLineWidth, color, false);
        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMax.fY, posMin.fZ), CVector(posMin.fX, posMax.fY, posMax.fZ), fLineWidth, color, false);

        // one corner
        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMin.fZ), CVector(posMin.fX, posMax.fY, posMin.fZ), fLineWidth, color, false);
        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMin.fZ), CVector(posMax.fX, posMin.fY, posMin.fZ), fLineWidth, color, false);
        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMax.fZ), CVector(posMin.fX, posMax.fY, posMax.fZ), fLineWidth, color, false);
        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMax.fZ), CVector(posMax.fX, posMin.fY, posMax.fZ), fLineWidth, color, false);

        // second corner
        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMin.fZ), CVector(posMin.fX, posMax.fY, posMin.fZ), fLineWidth, color, false);
        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMin.fZ), CVector(posMax.fX, posMin.fY, posMin.fZ), fLineWidth, color, false);
        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMax.fZ), CVector(posMin.fX, posMax.fY, posMax.fZ), fLineWidth, color, false);
        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMax.fZ), CVector(posMax.fX, posMin.fY, posMax.fZ), fLineWidth, color, false);
    }

    for (int i = 0; i < pColData->numColSpheres; i++)
    {
        pColSphere = &pColData->pColSpheres[i];
        spherePos = pColSphere->vecCenter + vecPosition;
        g_pCore->GetGraphics()->DrawWiredSphere(spherePos, pColSphere->fRadius, SColorARGB(255, 0, 0, 255), fLineWidth, 2);
    }

    // Bounding Box
    posMin = colModel->boundingBox.vecMin + vecPosition;
    posMax = colModel->boundingBox.vecMax + vecPosition;

    // vertical lines
    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMin.fZ), CVector(posMin.fX, posMin.fY, posMax.fZ), fLineWidth, color, false);
    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMin.fY, posMin.fZ), CVector(posMax.fX, posMin.fY, posMax.fZ), fLineWidth, color, false);
    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMin.fZ), CVector(posMax.fX, posMax.fY, posMax.fZ), fLineWidth, color, false);
    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMax.fY, posMin.fZ), CVector(posMin.fX, posMax.fY, posMax.fZ), fLineWidth, color, false);

    // one corner
    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMin.fZ), CVector(posMin.fX, posMax.fY, posMin.fZ), fLineWidth, color, false);
    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMin.fZ), CVector(posMax.fX, posMin.fY, posMin.fZ), fLineWidth, color, false);
    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMax.fZ), CVector(posMin.fX, posMax.fY, posMax.fZ), fLineWidth, color, false);
    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMax.fZ), CVector(posMax.fX, posMin.fY, posMax.fZ), fLineWidth, color, false);

    // second corner
    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMin.fZ), CVector(posMin.fX, posMax.fY, posMin.fZ), fLineWidth, color, false);
    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMin.fZ), CVector(posMax.fX, posMin.fY, posMin.fZ), fLineWidth, color, false);
    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMax.fZ), CVector(posMin.fX, posMax.fY, posMax.fZ), fLineWidth, color, false);
    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMax.fZ), CVector(posMax.fX, posMin.fY, posMax.fZ), fLineWidth, color, false);


    g_pCore->GetGraphics()->DrawWiredSphere(colModel->boundingBox.vecCenter + vecPosition, colModel->boundingBox.fRadius, SColorARGB(255, 0, 0, 255),
                                            fLineWidth, 2);
}

bool CCustomCollision::RemoveCustomCollision(CEntitySAInterface* pEntitySA)
{
    DWORD dwRef = (DWORD)pEntitySA;
    if (m_mapCustomCollisions.find(dwRef) != m_mapCustomCollisions.end())
    {
        delete m_mapCustomCollisions[dwRef];
        auto it = m_mapCustomCollisions.find(dwRef);
        m_mapCustomCollisions.erase(it);
        return true;
    }
    return false;
}

bool CCustomCollision::SetObjectCollision(CEntitySAInterface* pEntitySA, CColModelSAInterface* pColModelSA)
{
    DWORD dwRef = (DWORD)pEntitySA;
    if (m_mapCustomCollisions.find(dwRef) != m_mapCustomCollisions.end())
    {
        delete m_mapCustomCollisions[dwRef];
        auto it = m_mapCustomCollisions.find(dwRef);
        m_mapCustomCollisions.erase(it);
    }
    if (pColModelSA)
    {
        SaveOriginalCollision(pEntitySA->m_nModelIndex);
        CColModelSAInterface* pCopiedColModel = CopyColModel(pColModelSA);
        m_mapCustomCollisions.insert({dwRef, pCopiedColModel});
    }

    return true;
}

bool CCustomCollision::SetObjectScaledCollision(CEntitySAInterface* pEntitySA, CColModelSAInterface* pColModelSA)
{
    DWORD dwRef = (DWORD)pEntitySA;
    if (m_mapCustomScaledCollisions.find(dwRef) != m_mapCustomCollisions.end())
    {
        delete m_mapCustomScaledCollisions[dwRef];
        auto it = m_mapCustomScaledCollisions.find(dwRef);
        m_mapCustomScaledCollisions.erase(it);
    }
    if (pColModelSA)
    {
        SaveOriginalCollision(pEntitySA->m_nModelIndex);
        CColModelSAInterface* pCopiedColModel = CopyColModel(pColModelSA);
        m_mapCustomScaledCollisions.insert({dwRef, pCopiedColModel});
    }

    return true;
}

bool CCustomCollision::HasCustomCollision(CEntitySAInterface* pEntitySA)
{
    DWORD dwRef = (DWORD)pEntitySA;
    if (m_mapCustomCollisions.find(dwRef) != m_mapCustomCollisions.end())
        return true;

    if (m_mapCustomScaledCollisions.find(dwRef) != m_mapCustomScaledCollisions.end())
        return true;

    return false;
}

void CCustomCollision::SetObjectScale(CObject* pObject, CVector scale)
{
    CEntitySAInterface* pEntitySA = pObject->GetInterface();
    DWORD              dwRef = (DWORD)pEntitySA;
    SaveOriginalCollision(pEntitySA->m_nModelIndex);

    CColModelSAInterface* pCurrentCollision = GetCurrentCollision(pEntitySA, true);
    CColModelSAInterface* pCollisionCopy = CopyColModel(pCurrentCollision);
    ScaleCollision(pCollisionCopy, scale);
    if (scale.fX > 1.0f || scale.fY > 1.0f || scale.fZ > 1.0f)
    {
        CColModelSAInterface* pOriginalColModel = GetOriginalColModel(pEntitySA);
        pOriginalColModel->boundingBox.fRadius = 2;
        pOriginalColModel->boundingBox.vecCenter = CVector(2, 2, 2);
        pOriginalColModel->boundingBox.vecMin = CVector(-2, -2, -2);
        pOriginalColModel->boundingBox.vecMax = CVector(2, 2, 2);
    }
    SetObjectScaledCollision(pEntitySA, pCollisionCopy);
    int a = 5;
}

CColModelSAInterface* CCustomCollision::GetCurrentCollision(CObject* pObject, bool bSkipScaled)
{
    CEntitySAInterface* pEntitySA = pObject->GetInterface();
    return GetCurrentCollision(pEntitySA, bSkipScaled);
}

CColModelSAInterface* CCustomCollision::GetCurrentCollision(CEntitySAInterface* pEntitySA, bool bSkipScaled)
{
    DWORD dwInterface = (DWORD)pEntitySA;
    if (bSkipScaled)
    {
        if (m_mapCustomCollisions.find(dwInterface) == m_mapCustomCollisions.end())
        {
            return GetOriginalColModel(pEntitySA);
        }

        return m_mapCustomCollisions[dwInterface];
    }
    else
    {
        if (m_mapCustomScaledCollisions.find(dwInterface) != m_mapCustomScaledCollisions.end())
        {
            return m_mapCustomScaledCollisions[dwInterface];
        }
        if (m_mapCustomCollisions.find(dwInterface) != m_mapCustomCollisions.end())
        {
            return m_mapCustomCollisions[dwInterface];
        }

        return GetOriginalColModel(pEntitySA);
    }
}
/*
    CObjectSAInterface* pInterface = static_cast<CObjectSAInterface*>(pEntity);

    //g_pCore->GetGame()->GetClock
    SClientEntity<CObjectSA>* pObjectClientEntity = g_pCore->GetGame()->GetPools()->GetObjectA((DWORD*)pInterface);

    CClientEntity* pClientEntity = pObjectClientEntity->pClientEntity;
    CObjectSA*                pObject = pObjectClientEntity ? pObjectClientEntity->pEntity : nullptr;
    CColModelSAInterface* pCol = GetScaledCollision(pObject);
    //DrawCollisionDebug(pObject);
*/
