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
#include "./game/CObject.h"


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
    pColModel->m_sphere.m_radius *= std::max(scale.fX, std::max(scale.fY, scale.fZ)) * 2;
    pColModel->m_sphere.m_center *= (scale / 2);
    pColModel->m_bounds.m_vecMax *= scale;
    pColModel->m_bounds.m_vecMin *= scale;

    for (int i = 0; i < pColModel->m_data->m_numSpheres; i++)
    {
        pColModel->m_data->m_spheres[i].m_radius *= scale.fX;
        pColModel->m_data->m_spheres[i].m_center *= scale;
    }

    for (int i = 0; i < pColModel->m_data->m_numBoxes; i++)
    {
        pColModel->m_data->m_boxes[i].m_vecMax *= scale;
        pColModel->m_data->m_boxes[i].m_vecMin *= scale;
    }
    for (int i = 0; i < pColModel->m_data->getNumVertices(); i++)
    {
        pColModel->m_data->m_vertices[i].x *= scale.fX;
        pColModel->m_data->m_vertices[i].y *= scale.fY;
        pColModel->m_data->m_vertices[i].z *= scale.fZ;
    }
}

CColModelSAInterface* CCustomCollision::CopyColModel(CColModelSAInterface* pColModel)
{
    CColModelSAInterface* pInterface = new CColModelSAInterface();
    pInterface->m_bounds = pColModel->m_bounds;
    pInterface->m_data = new CColDataSA();
    pInterface->m_data->m_numSpheres = pColModel->m_data->m_numSpheres;
    pInterface->m_data->m_numBoxes = pColModel->m_data->m_numBoxes;
    pInterface->m_data->m_numTriangles = pColModel->m_data->m_numTriangles;
    pInterface->m_data->m_numSuspensionLines = pColModel->m_data->m_numSuspensionLines;
    pInterface->m_data->m_numShadowTriangles = pColModel->m_data->m_numShadowTriangles;
    pInterface->m_data->m_numShadowVertices = pColModel->m_data->m_numShadowVertices;
    pInterface->m_data->m_flags = pColModel->m_data->m_flags;

    if (pInterface->m_data->m_numShadowVertices > 0)
    {
        pInterface->m_data->m_shadowVertices = new CCompressedVectorSA[pInterface->m_data->m_numShadowTriangles];
        memcpy(pInterface->m_data->m_shadowVertices, pInterface->m_data->m_shadowVertices,
               sizeof(CCompressedVectorSA) * pInterface->m_data->m_numShadowTriangles);
    }

    if (pInterface->m_data->m_numShadowTriangles > 0)
    {
        pInterface->m_data->m_shadowTriangles = new CColTriangleSA[pInterface->m_data->m_numShadowTriangles];
        memcpy(pInterface->m_data->m_shadowTriangles, pColModel->m_data->m_shadowTriangles, sizeof(CColTriangleSA) * pInterface->m_data->m_numShadowTriangles);
    }

    if (pInterface->m_data->m_numSpheres > 0)
    {
        pInterface->m_data->m_spheres = new CColSphereSA[pInterface->m_data->m_numSpheres];
        memcpy(pInterface->m_data->m_spheres, pColModel->m_data->m_spheres, sizeof(CColSphereSA) * pInterface->m_data->m_numSpheres);
    }

    if (pInterface->m_data->m_numBoxes > 0)
    {
        pInterface->m_data->m_boxes = new CColBoxSA[pInterface->m_data->m_numBoxes];
        memcpy(pInterface->m_data->m_boxes, pColModel->m_data->m_boxes, sizeof(CColBoxSA) * pInterface->m_data->m_numBoxes);
    }

    if (pInterface->m_data->m_numTriangles > 0)
    {
        pInterface->m_data->m_triangles = new CColTriangleSA[pInterface->m_data->m_numTriangles];
        memcpy(pInterface->m_data->m_triangles, pColModel->m_data->m_triangles, sizeof(CColTriangleSA) * pInterface->m_data->m_numTriangles);
    }

    //if (pInterface->m_data->numColTriangles > 0)
    //{
    //    pInterface->m_data->pColTrianglePlanes = new CColTrianglePlaneSA[pInterface->m_data->numColTriangles];
    //    memcpy(pInterface->m_data->pColTrianglePlanes, pColModel->m_data->pColTrianglePlanes,
    //           sizeof(CColTrianglePlaneSA) * pInterface->m_data->numColTriangles);
    //}
    pInterface->m_data->m_trianglePlanes = nullptr;

    if (pInterface->m_data->m_numSuspensionLines > 0)
    {
        pInterface->m_data->m_suspensionLines = new CColLineSA[pInterface->m_data->m_numSuspensionLines];
        memcpy(pInterface->m_data->m_suspensionLines, pColModel->m_data->m_suspensionLines, sizeof(CColLineSA) * pInterface->m_data->m_numSuspensionLines);
    }

    ushort numVertices = pColModel->m_data->getNumVertices();
    if (numVertices > 0)
    {
        pInterface->m_data->m_vertices = new CCompressedVectorSA[numVertices];
        memcpy(pInterface->m_data->m_vertices, pColModel->m_data->m_vertices, sizeof(CCompressedVectorSA) * numVertices);
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
    CColDataSA*           m_data = colModel->m_data;

    if (m_data == nullptr)
        return;

    CVector vecPosition = *pObject->GetPosition();

    CVector      bboxMin = colModel->m_bounds.m_vecMin;
    CVector      bboxMax = colModel->m_bounds.m_vecMax;
    unsigned int color = (unsigned int)colModel;            // make color depends on collision
    color |= 0x000000ff;                                    // with alpha 255

    // g_pCore->GetGraphics()->DrawLine3DQueued(vecPosition - bboxMin, vecPosition + bboxMax, 10, (unsigned int)colModel,false);

    float fLineWidth = 4.f + pow(colModel->m_sphere.m_radius, 0.5f);

    CVector       trianglePosition[3];
    CVector       posMin;
    CVector       posMax;
    CColBoxSA*    pColBox;
    CColSphereSA* pColSphere;
    CVector       spherePos;

    for (int i = 0; i < m_data->m_numTriangles; i++)
    {
        CColTriangleSA* pColTriangle = &m_data->m_triangles[i];
        trianglePosition[0] = m_data->m_vertices[pColTriangle->m_indices[0]].getVector() + vecPosition;
        trianglePosition[1] = m_data->m_vertices[pColTriangle->m_indices[1]].getVector() + vecPosition;
        trianglePosition[2] = m_data->m_vertices[pColTriangle->m_indices[2]].getVector() + vecPosition;
        g_pCore->GetGraphics()->DrawLine3DQueued(trianglePosition[0], trianglePosition[1], fLineWidth, color);
        g_pCore->GetGraphics()->DrawLine3DQueued(trianglePosition[1], trianglePosition[2], fLineWidth, color);
        g_pCore->GetGraphics()->DrawLine3DQueued(trianglePosition[2], trianglePosition[0], fLineWidth, color);
    }

    for (int i = 0; i < m_data->m_numBoxes; i++)
    {
        pColBox = &m_data->m_boxes[i];
        posMin = pColBox->m_vecMin + vecPosition;
        posMax = pColBox->m_vecMax + vecPosition;

        // vertical lines
        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMin.fZ), CVector(posMin.fX, posMin.fY, posMax.fZ), fLineWidth, color);
        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMin.fY, posMin.fZ), CVector(posMax.fX, posMin.fY, posMax.fZ), fLineWidth, color);
        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMin.fZ), CVector(posMax.fX, posMax.fY, posMax.fZ), fLineWidth, color);
        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMax.fY, posMin.fZ), CVector(posMin.fX, posMax.fY, posMax.fZ), fLineWidth, color);

        // one corner
        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMin.fZ), CVector(posMin.fX, posMax.fY, posMin.fZ), fLineWidth, color);
        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMin.fZ), CVector(posMax.fX, posMin.fY, posMin.fZ), fLineWidth, color);
        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMax.fZ), CVector(posMin.fX, posMax.fY, posMax.fZ), fLineWidth, color);
        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMax.fZ), CVector(posMax.fX, posMin.fY, posMax.fZ), fLineWidth, color);

        // second corner
        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMin.fZ), CVector(posMin.fX, posMax.fY, posMin.fZ), fLineWidth, color);
        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMin.fZ), CVector(posMax.fX, posMin.fY, posMin.fZ), fLineWidth, color);
        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMax.fZ), CVector(posMin.fX, posMax.fY, posMax.fZ), fLineWidth, color);
        g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMax.fZ), CVector(posMax.fX, posMin.fY, posMax.fZ), fLineWidth, color);
    }

    for (int i = 0; i < m_data->m_numSpheres; i++)
    {
        pColSphere = &m_data->m_spheres[i];
        spherePos = pColSphere->m_center + vecPosition;
        g_pCore->GetGraphics()->DrawWiredSphere(spherePos, pColSphere->m_radius, SColorARGB(255, 0, 0, 255), fLineWidth, 2);
    }

    // Bounding Box
    posMin = colModel->m_bounds.m_vecMin + vecPosition;
    posMax = colModel->m_bounds.m_vecMax + vecPosition;

    // vertical lines
    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMin.fZ), CVector(posMin.fX, posMin.fY, posMax.fZ), fLineWidth, color);
    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMin.fY, posMin.fZ), CVector(posMax.fX, posMin.fY, posMax.fZ), fLineWidth, color);
    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMin.fZ), CVector(posMax.fX, posMax.fY, posMax.fZ), fLineWidth, color);
    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMax.fY, posMin.fZ), CVector(posMin.fX, posMax.fY, posMax.fZ), fLineWidth, color);

    // one corner
    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMin.fZ), CVector(posMin.fX, posMax.fY, posMin.fZ), fLineWidth, color);
    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMin.fZ), CVector(posMax.fX, posMin.fY, posMin.fZ), fLineWidth, color);
    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMax.fZ), CVector(posMin.fX, posMax.fY, posMax.fZ), fLineWidth, color);
    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMin.fX, posMin.fY, posMax.fZ), CVector(posMax.fX, posMin.fY, posMax.fZ), fLineWidth, color);

    // second corner
    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMin.fZ), CVector(posMin.fX, posMax.fY, posMin.fZ), fLineWidth, color);
    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMin.fZ), CVector(posMax.fX, posMin.fY, posMin.fZ), fLineWidth, color);
    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMax.fZ), CVector(posMin.fX, posMax.fY, posMax.fZ), fLineWidth, color);
    g_pCore->GetGraphics()->DrawLine3DQueued(CVector(posMax.fX, posMax.fY, posMax.fZ), CVector(posMax.fX, posMin.fY, posMax.fZ), fLineWidth, color);


    g_pCore->GetGraphics()->DrawWiredSphere(colModel->m_sphere.m_center + vecPosition, colModel->m_sphere.m_radius, SColorARGB(255, 0, 0, 255),
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
        pOriginalColModel->m_sphere.m_radius = 2;
        pOriginalColModel->m_sphere.m_center = CVector(2, 2, 2);
        pOriginalColModel->m_bounds.m_vecMin = CVector(-2, -2, -2);
        pOriginalColModel->m_bounds.m_vecMax = CVector(2, 2, 2);
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
