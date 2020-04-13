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

void CCustomCollision::SaveOriginalCollisionIfDoesNotExists(WORD model, CColModelSAInterface* colModel)
{
    auto origCol = originalCollisions.find(model);

    if (origCol == originalCollisions.end())
    {
        CColDataSA* colData = colModel->pColData;
        if (colData == nullptr)
            return;

        OriginalCollisions* original = new OriginalCollisions();
        ushort              verticesCount = colData->getNumVertices();

        original->spheresCount = colData->numColSpheres;
        original->boxesCount = colData->numColBoxes;
        original->verticesCount = verticesCount;

        original->spheres = new CColSphereSA[original->spheresCount]{};
        original->boxes = new CColBoxSA[original->boxesCount]{};
        original->vertices = new CompressedVector[original->verticesCount]{};

        memcpy(&original->boundingBox, &colModel->boundingBox, sizeof(CBoundingBoxSA));

        for (int i = 0; i < original->spheresCount; i++)
            memcpy(&original->spheres[i], &colData->pColSpheres[i], sizeof(CColSphereSA));

        for (int i = 0; i < original->boxesCount; i++)
            memcpy(&original->boxes[i], &colData->pColBoxes[i], sizeof(CColBoxSA));

        for (int i = 0; i < original->verticesCount; i++)
            memcpy(&original->spheres[i], &colData->pVertices[i], sizeof(CompressedVector));

        originalCollisions[model] = original;
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

ScaledCollision* CCustomCollision::GetScaledCollision(WORD model, CVector* targetScale)
{
    OriginalCollisions* original = originalCollisions[model];
    auto                scaledCols = scaledCollisions.find(model);

    std::vector<ScaledCollision*> vecScaledCollisions;

    // if this model doesn't get scaled
    if (scaledCols == scaledCollisions.end())
    {
        vecScaledCollisions = std::vector<ScaledCollision*>();
        scaledCollisions[model] = vecScaledCollisions;
    }

    vecScaledCollisions = scaledCollisions[model];
    for (ScaledCollision* scaledCollision : vecScaledCollisions)
    {
        if (scaledCollision->scale->fX == targetScale->fX && scaledCollision->scale->fY == targetScale->fY && scaledCollision->scale->fZ == targetScale->fZ)
        {
            return scaledCollision;
        }
    }

    // scale for this wasn't used yet, so create it

    ScaledCollision* scaledCollision = new ScaledCollision();
    scaledCollisions[model].push_back(scaledCollision);
    scaledCollision->scale = targetScale;
    /////////////////////
    scaledCollision->spheresCount = original->spheresCount;
    scaledCollision->boxesCount = original->boxesCount;
    scaledCollision->verticesCount = original->verticesCount;

    scaledCollision->spheres = new CColSphereSA[original->spheresCount]{};
    scaledCollision->boxes = new CColBoxSA[original->boxesCount]{};
    scaledCollision->vertices = new CompressedVector[original->verticesCount]{};

    memcpy(&scaledCollision->boundingBox, &original->boundingBox, sizeof(CBoundingBoxSA));

    for (int i = 0; i < original->spheresCount; i++)
        scaledCollision->spheres[i] = original->spheres[i];

    for (int i = 0; i < original->boxesCount; i++)
        scaledCollision->boxes[i] = original->boxes[i];

    for (int i = 0; i < original->verticesCount; i++)
        scaledCollision->vertices[i] = original->vertices[i];

    ////////////////////////////

    memcpy(&scaledCollision->boundingBox, &original->boundingBox, sizeof(CBoundingBoxSA));

    scaledCollision->boundingBox.fRadius *= std::max(targetScale->fX, std::max(targetScale->fY, targetScale->fZ));
    scaledCollision->boundingBox.vecCenter *= (*targetScale / 2);
    scaledCollision->boundingBox.vecMax *= *targetScale;
    scaledCollision->boundingBox.vecMin *= *targetScale;

    for (int i = 0; i < scaledCollision->spheresCount; i++)
    {
        scaledCollision->spheres[i].fRadius *= targetScale->fX;
        scaledCollision->spheres[i].vecCenter *= *targetScale;
    }

    for (int i = 0; i < scaledCollision->boxesCount; i++)
    {
        scaledCollision->boxes[i].max *= *targetScale;
        scaledCollision->boxes[i].min *= *targetScale;
    }
    for (int i = 0; i < original->verticesCount; i++)
    {
        scaledCollision->vertices[i].x *= targetScale->fX;
        scaledCollision->vertices[i].y *= targetScale->fY;
        scaledCollision->vertices[i].z *= targetScale->fZ;
    }

    return scaledCollision;
}

CColModelSAInterface* CCustomCollision::GetScaled(WORD model, CColModelSAInterface* colModel, CVector* scale)
{
    if (colModel->pColData == nullptr)
        return colModel;

    SaveOriginalCollisionIfDoesNotExists(model, colModel);

    auto origCol = originalCollisions.find(model);

    if (origCol == originalCollisions.end())
        return colModel;

    if (scale->fX == 1 && scale->fY == 1 && scale->fZ == 1)
    {
        OriginalCollisions* original = originalCollisions[model];
        if (colModel->pColData->pColSpheres != nullptr)
            colModel->pColData->pColSpheres = original->spheres;
        if (colModel->pColData->pColBoxes != nullptr)
            colModel->pColData->pColBoxes = original->boxes;
        if (colModel->pColData->pVertices != nullptr)
            colModel->pColData->pVertices = original->vertices;

        colModel->boundingBox = original->boundingBox;
    }
    else
    {
        ScaledCollision* col = GetScaledCollision(model, scale);
        if (colModel->pColData->pColSpheres != nullptr)
            colModel->pColData->pColSpheres = col->spheres;
        if (colModel->pColData->pColBoxes != nullptr)
            colModel->pColData->pColBoxes = col->boxes;
        if (colModel->pColData->pVertices != nullptr)
            colModel->pColData->pVertices = col->vertices;

        colModel->boundingBox = col->boundingBox;
    }

    return colModel;
}

CColModelSAInterface* CCustomCollision::GetScaledCollision(CObject* pObject)
{
    //CModelInfoSA*         pModelInfoSA = (CModelInfoSA*)(pGame->GetModelInfo(pObject->GetModelIndex()));
    //CColModelSAInterface* colModel = pModelInfoSA->GetInterface()->pColModel;

    //colModel->boundingBox.vecMax *= 10;
    //colModel->boundingBox.vecMin *= 10;
    //colModel->boundingBox.fRadius *= 10;
    //return GetScaled(pObject->GetModelIndex(), colModel, pObject->GetScale());
    return nullptr;
}
//
//void DrawCollisionDebug(CObject* pObject)
//{
//    CColModelSAInterface* colModel = GetScaledCollision(pObject);
//    CColDataSA*           pColData = colModel->pColData;
//
//    if (pColData == nullptr)
//        return;
//
//    CVector vecPosition = *pObject->GetPosition();
//
//    CVector      bboxMin = colModel->boundingBox.vecMin;
//    CVector      bboxMax = colModel->boundingBox.vecMax;
//    unsigned int color = (unsigned int)colModel;            // make color depends on collision
//    color |= 0x000000ff;                                    // with alpha 255
//
//    // g_pCore->GetGraphics()->DrawLine3DQueued(vecPosition - bboxMin, vecPosition + bboxMax, 10, (unsigned int)colModel,false);
//
//    float fLineWidth = 4.f + pow(colModel->boundingBox.fRadius, 0.5f);
//
//    CVector       trianglePosition[3];
//    CVector       posMin;
//    CVector       posMax;
//    CColBoxSA*    pColBox;
//    CColSphereSA* pColSphere;
//    CVector       spherePos;
//
//    for (int i = 0; i < pColData->numColTriangles; i++)
//    {
//        CColTriangleSA* pColTriangle = &pColData->pColTriangles[i];
//        trianglePosition[0] = pColData->pVertices[pColTriangle->v1].getVector() + vecPosition;
//        trianglePosition[1] = pColData->pVertices[pColTriangle->v2].getVector() + vecPosition;
//        trianglePosition[2] = pColData->pVertices[pColTriangle->v3].getVector() + vecPosition;
//        g_pCore->GetGraphics()->DrawLine3DQueued(trianglePosition[0], trianglePosition[1], fLineWidth, color, false);
//        g_pCore->GetGraphics()->DrawLine3DQueued(trianglePosition[1], trianglePosition[2], fLineWidth, color, false);
//        g_pCore->GetGraphics()->DrawLine3DQueued(trianglePosition[2], trianglePosition[0], fLineWidth, color, false);
//    }
//
//    for (int i = 0; i < pColData->numColBoxes; i++)
//    {
//        pColBox = &pColData->pColBoxes[i];
//        posMin = pColBox->min + vecPosition;
//        posMax = pColBox->max + vecPosition;
//
//        // vertical lines
//        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMin.fZ), CVector(posMin.fX, posMin.fY, posMax.fZ), fLineWidth, color, false);
//        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMin.fY, posMin.fZ), CVector(posMax.fX, posMin.fY, posMax.fZ), fLineWidth, color, false);
//        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMin.fZ), CVector(posMax.fX, posMax.fY, posMax.fZ), fLineWidth, color, false);
//        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMax.fY, posMin.fZ), CVector(posMin.fX, posMax.fY, posMax.fZ), fLineWidth, color, false);
//
//        // one corner
//        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMin.fZ), CVector(posMin.fX, posMax.fY, posMin.fZ), fLineWidth, color, false);
//        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMin.fZ), CVector(posMax.fX, posMin.fY, posMin.fZ), fLineWidth, color, false);
//        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMax.fZ), CVector(posMin.fX, posMax.fY, posMax.fZ), fLineWidth, color, false);
//        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMax.fZ), CVector(posMax.fX, posMin.fY, posMax.fZ), fLineWidth, color, false);
//
//        // second corner
//        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMin.fZ), CVector(posMin.fX, posMax.fY, posMin.fZ), fLineWidth, color, false);
//        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMin.fZ), CVector(posMax.fX, posMin.fY, posMin.fZ), fLineWidth, color, false);
//        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMax.fZ), CVector(posMin.fX, posMax.fY, posMax.fZ), fLineWidth, color, false);
//        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMax.fZ), CVector(posMax.fX, posMin.fY, posMax.fZ), fLineWidth, color, false);
//    }
//
//    for (int i = 0; i < pColData->numColSpheres; i++)
//    {
//        pColSphere = &pColData->pColSpheres[i];
//        spherePos = pColSphere->vecCenter + vecPosition;
//        // DrawSphere(spherePos, pColSphere->fRadius, color);
//    }
//
//    // Bounding Box
//    posMin = colModel->boundingBox.vecMin + vecPosition;
//    posMax = colModel->boundingBox.vecMax + vecPosition;
//
//    // vertical lines
//    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMin.fZ), CVector(posMin.fX, posMin.fY, posMax.fZ), fLineWidth, color, false);
//    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMin.fY, posMin.fZ), CVector(posMax.fX, posMin.fY, posMax.fZ), fLineWidth, color, false);
//    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMin.fZ), CVector(posMax.fX, posMax.fY, posMax.fZ), fLineWidth, color, false);
//    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMax.fY, posMin.fZ), CVector(posMin.fX, posMax.fY, posMax.fZ), fLineWidth, color, false);
//
//    // one corner
//    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMin.fZ), CVector(posMin.fX, posMax.fY, posMin.fZ), fLineWidth, color, false);
//    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMin.fZ), CVector(posMax.fX, posMin.fY, posMin.fZ), fLineWidth, color, false);
//    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMax.fZ), CVector(posMin.fX, posMax.fY, posMax.fZ), fLineWidth, color, false);
//    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMax.fZ), CVector(posMax.fX, posMin.fY, posMax.fZ), fLineWidth, color, false);
//
//    // second corner
//    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMin.fZ), CVector(posMin.fX, posMax.fY, posMin.fZ), fLineWidth, color, false);
//    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMin.fZ), CVector(posMax.fX, posMin.fY, posMin.fZ), fLineWidth, color, false);
//    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMax.fZ), CVector(posMin.fX, posMax.fY, posMax.fZ), fLineWidth, color, false);
//    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMax.fZ), CVector(posMax.fX, posMin.fY, posMax.fZ), fLineWidth, color, false);
//
//    // DrawSphere(colModel->boundingBox.vecCenter + vecPosition, colModel->boundingBox.fRadius, color);
//}

bool CCustomCollision::SetObjectCollision(CEntitySAInterface* pEntitySA, CColModelSAInterface* pColModelSA)
{
    DWORD              dwRef = (DWORD)pEntitySA;
    SaveOriginalCollision(pEntitySA->m_nModelIndex);
    if (m_mapCustomCollisions.find(dwRef) != m_mapCustomCollisions.end())
    {
        delete m_mapCustomCollisions[dwRef];
        auto it = m_mapCustomCollisions.find(dwRef);
        m_mapCustomCollisions.erase(it);
    }
    if (pColModelSA)
    {
        CColModelSAInterface* pCopiedColModel = CopyColModel(pColModelSA);
        m_mapCustomCollisions.insert({dwRef, pCopiedColModel});
    }

    return true;
}

void CCustomCollision::SetObjectScale(CEntitySAInterface* pEntitySA, CVector scale)
{
    DWORD              dwRef = (DWORD)pEntitySA;
    SaveOriginalCollision(pEntitySA->m_nModelIndex);
    if (m_mapCustomCollisions.find(pEntitySA->m_nModelIndex) == m_mapCustomCollisions.end())
    {
    }
    //GetOriginalColModel(pObject->GetInterface());

    int a = 5;
}

CColModelSAInterface* CCustomCollision::GetCustomCollision(CObject* pObject)
{
    DWORD dwInterface = (DWORD)pObject->GetInterface();
    if (m_mapCustomCollisions.find(dwInterface) == m_mapCustomCollisions.end())
    {
        SClientEntity<CObjectSA>* pObjectClientEntity = g_pCore->GetGame()->GetPools()->GetObjectA((DWORD*)pObject->GetInterface());
        CObjectSA*                pObjectSA = pObjectClientEntity ? pObjectClientEntity->pEntity : nullptr;
        return GetOriginalColModel(pObject->GetInterface());
    }
    return m_mapCustomCollisions[dwInterface];
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
