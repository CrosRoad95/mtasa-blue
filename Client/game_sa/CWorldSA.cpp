/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        game_sa/CWorldSA.cpp
 *  PURPOSE:     Game world/entity logic
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#include "StdInc.h"

#define ARRAY_SurfaceInfos 0xB79538

namespace
{
    extern const unsigned char aOriginalSurfaceInfo[2292];
}

CWorldSA::CWorldSA()
{
    m_pBuildingRemovals = new std::multimap<unsigned short, SBuildingRemoval*>;
    m_pDataBuildings = new std::multimap<unsigned short, sDataBuildingRemovalItem*>;
    m_pBinaryBuildings = new std::multimap<unsigned short, sBuildingRemovalItem*>;

    m_pSurfaceInfo = reinterpret_cast<CSurfaceType*>(ARRAY_SurfaceInfos);

    InstallHooks();
}

CSurfaceType* CWorldSA::GetSurfaceInfo()
{
    return m_pSurfaceInfo;
}

void CWorldSA::ResetAllSurfaceInfo()
{
#if 0
    // Verification code for the embedded surface infos byte array:
    // GTA's surface infos are not embedded in the binary. You have to connect to a server
    // and then disconnect (the server MUST NOT modify the surface infos) to run the code below.
    // Checks if surface infos are byte-equal to the embedded original surface info bytes
    assert(memcmp(m_pSurfaceInfo, aOriginalSurfaceInfo, sizeof(CSurfaceType)) == 0);
#endif

    const auto iSurfaceInfo = reinterpret_cast<intptr_t>(m_pSurfaceInfo);
    const auto iOriginalSurfaceInfo = reinterpret_cast<intptr_t>(&aOriginalSurfaceInfo[0]);

    const size_t sOffset = offsetof(CSurfaceType, CSurfaceType::surfType);
    const size_t sSize = sizeof(CSurfaceType) - sOffset;

    memcpy((void*)(iSurfaceInfo + sOffset), (void*)(iOriginalSurfaceInfo + sOffset), sSize);
}

bool CWorldSA::ResetSurfaceInfo(short sSurfaceID)
{
    if (sSurfaceID >= EColSurfaceValue::DEFAULT && sSurfaceID <= EColSurfaceValue::SIZE)
    {
        const auto iSurfaceInfo = reinterpret_cast<intptr_t>(m_pSurfaceInfo);
        const auto iOriginalSurfaceInfo = reinterpret_cast<intptr_t>(&aOriginalSurfaceInfo[0]);

        const size_t sOffset = offsetof(CSurfaceType, CSurfaceType::surfType) + sizeof(SurfaceInfo_c) * sSurfaceID;
        const size_t sSize = sizeof(SurfaceInfo_c);

        memcpy((void*)(iSurfaceInfo + sOffset), (void*)(iOriginalSurfaceInfo + sOffset), sSize);
        return true;
    }

    return false;
}

void HOOK_FallenPeds();
void HOOK_FallenCars();
void HOOK_GetColModel();
void HOOK_GetBoundRect();

void CWorldSA::InstallHooks()
{
    HookInstall(0x565CB0, (DWORD)HOOK_FallenPeds, 5);
    HookInstall(0x565E80, (DWORD)HOOK_FallenCars, 5);
    HookInstall(0x535300, (DWORD)HOOK_GetColModel, 5);
    HookInstall(0x53412A, (DWORD)HOOK_GetBoundRect, 7);
}

DWORD CONTINUE_CWorld_FallenPeds = 0x00565CBA;
DWORD CONTINUE_CWorld_FallenCars = 0x00565E8A;
DWORD CONTINUE_CWorld_GetColModel = 0x00535305;
DWORD CONTINUE_CWorld_GetBoundRect = 0x00534131;

bool                  bIsObject = false;
CColModelSAInterface* colModel;
CObject*              pObject;

void GetColModel(CEntitySAInterface* pEntity)
{
    bIsObject = false;
    if (pEntity->nType == ENTITY_TYPE_OBJECT)
    {
        // g_pCore->GetConsole()->Printf("Model %i", pEntity->m_nModelIndex);
        CObjectSAInterface* pInterface = static_cast<CObjectSAInterface*>(pEntity);

        SClientEntity<CObjectSA>* pObjectClientEntity = g_pCore->GetGame()->GetPools()->GetObjectA((DWORD*)pInterface);
        pObject = pObjectClientEntity ? pObjectClientEntity->pEntity : nullptr;
        if (pObject != nullptr)
        {
            bIsObject = true;
        }
    }
}

class OriginalCollisions
{
public:
    CColSphereSA*     spheres;
    CColBoxSA*        boxes;
    CompressedVector* vertices;
    ushort            spheresCount;
    ushort            boxesCount;
    ushort            verticesCount;
    CBoundingBoxSA    boundingBox;
};

class ScaledCollision
{
public:
    CVector*          scale;
    CColSphereSA*     spheres;
    CColBoxSA*        boxes;
    CompressedVector* vertices;
    ushort            spheresCount;
    ushort            boxesCount;
    ushort            verticesCount;
    CBoundingBoxSA    boundingBox;
};

std::map<WORD, std::vector<ScaledCollision*>> scaledCollisions = std::map<WORD, std::vector<ScaledCollision*>>();
std::map<WORD, OriginalCollisions*>           originalCollisions = std::map<WORD, OriginalCollisions*>();

void SaveOriginalCollisionIfDoesNotExists(WORD model, CColModelSAInterface* colModel)
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

ScaledCollision* GetScaledCollision(WORD model, CVector* targetScale)
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

CColModelSAInterface* GetScaled(WORD model, CColModelSAInterface* colModel, CVector* scale)
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

CColModelSAInterface* GetScaledCollision(CObject* pObject)
{
    CModelInfoSA*         pModelInfoSA = (CModelInfoSA*)(pGame->GetModelInfo(pObject->GetModelIndex()));
    CColModelSAInterface* colModel = pModelInfoSA->GetInterface()->pColModel;

    return GetScaled(pObject->GetModelIndex(), colModel, pObject->GetScale());
}

namespace
{
    //
    // CClientColSphere helpers
    //

    // Used in CreateSphereFaces
    struct SFace
    {
        SFace(const CVector& a, const CVector& b, const CVector& c) : a(a), b(b), c(c) {}
        CVector a, b, c;
    };

    //
    // Create faces for a sphere
    //
    void CreateSphereFaces(std::vector<SFace>& faceList, int iIterations)
    {
        int numFaces = (int)(pow(4.0, iIterations) * 8);
        faceList.clear();
        faceList.reserve(numFaces);

        // Initial octahedron
        static SFixedArray<CVector, 6>     vecPoints = {CVector(0, 0, 1),  CVector(0, 0, -1), CVector(-1, -1, 0),
                                                    CVector(1, -1, 0), CVector(1, 1, 0),  CVector(-1, 1, 0)};
        static const SFixedArray<WORD, 24> indices = {0, 3, 4, 0, 4, 5, 0, 5, 2, 0, 2, 3, 1, 4, 3, 1, 5, 4, 1, 2, 5, 1, 3, 2};

        for (uint i = 0; i < NUMELMS(vecPoints); i++)
            vecPoints[i].Normalize();

        for (uint i = 0; i < NUMELMS(indices); i += 3)
            faceList.push_back(SFace(vecPoints[indices[i]], vecPoints[indices[i + 1]], vecPoints[indices[i + 2]]));

        // For each iteration
        while (iIterations--)
        {
            // Divide each face into 4
            for (int i = faceList.size() - 1; i >= 0; i--)
            {
                // Get the three face points
                CVector a = faceList[i].a;
                CVector b = faceList[i].b;
                CVector c = faceList[i].c;

                // Make three inner points
                CVector a2 = (a + b) * 0.5f;
                CVector b2 = (b + c) * 0.5f;
                CVector c2 = (c + a) * 0.5f;

                // Keep points to the edge of the unit sphere
                a2.Normalize();
                b2.Normalize();
                c2.Normalize();

                // Replace the original face with the first sub-face
                faceList[i] = SFace(a2, b2, c2);

                // Add the three other sub-faces to the end of the list
                faceList.push_back(SFace(a, a2, c2));
                faceList.push_back(SFace(b, a2, b2));
                faceList.push_back(SFace(c, b2, c2));
            }
        }
    }

    //
    // For holding the lines of an optimized wireframe sphere
    //
    struct SWireModel
    {
        std::vector<CVector> vertexList;

        // Add a line if it is unique
        void AddLine(const CVector& from, const CVector& to)
        {
            // Conform order
            if (from.fX > to.fX || (from.fX == to.fX && from.fY > to.fY || (from.fY == to.fY && from.fZ > to.fZ)))
                return AddLine(to, from);

            // Find existing line
            for (int i = 0; i < (int)vertexList.size() - 1; i += 2)
                if ((from - vertexList[i]).LengthSquared() < 0.00001f)
                    if ((to - vertexList[i + 1]).LengthSquared() < 0.00001f)
                        return;            // Duplicated

            // Add new line
            vertexList.push_back(from);
            vertexList.push_back(to);
        }
    };

    const SWireModel& GetSphereWireModel(int iterations)
    {
        static std::map<uint, SWireModel> wireModelMap;

        // Find existing
        SWireModel* pWireModel = MapFind(wireModelMap, iterations);
        if (pWireModel)
            return *pWireModel;

        // Add new
        MapSet(wireModelMap, iterations, SWireModel());
        SWireModel& wireModel = *MapFind(wireModelMap, iterations);

        std::vector<SFace> faceList;
        CreateSphereFaces(faceList, iterations);

        // Create big vertex/line list
        for (uint i = 0; i < faceList.size(); i++)
        {
            wireModel.AddLine(faceList[i].a, faceList[i].b);
            wireModel.AddLine(faceList[i].b, faceList[i].c);
            wireModel.AddLine(faceList[i].c, faceList[i].a);
        }
        return wireModel;
    }
}            // namespace

void DrawSphere(CVector pos, float radius, unsigned int color)
{
    CGraphicsInterface* pGraphics = g_pCore->GetGraphics();

    float fLineWidth = 4.f + pow(radius, 0.5f);
    uint  iterations = Clamp(1, Round(pow(radius, 0.25f) * 0.75f), 4);

    const SWireModel& model = GetSphereWireModel(iterations);

    for (uint i = 0; i < model.vertexList.size(); i += 2)
    {
        const CVector& vecBegin = model.vertexList[i] * radius + pos;
        const CVector& vecEnd = model.vertexList[i + 1] * radius + pos;
        pGraphics->DrawLine3DQueued(vecBegin, vecEnd, fLineWidth, color, false);
    }
}

void DrawCollisionDebug(CObject* pObject)
{
    CColModelSAInterface* colModel = GetScaledCollision(pObject);
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
        DrawSphere(spherePos, pColSphere->fRadius, color);
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

    DrawSphere(colModel->boundingBox.vecCenter + vecPosition, colModel->boundingBox.fRadius, color);
}

CColModelSAInterface* DoSomethingWithCollision(CObject* pObject)
{
    CColModelSAInterface* pCol = GetScaledCollision(pObject);
    DrawCollisionDebug(pObject);
    return pCol;
}

typedef struct
{
    float left;
    float bottom;
    float right;
    float top;
} CRect;

CRect* rectRet;


bool bGotReplaced = false;
CColModelSAInterface* pColModelReplace;

void GetBoundRect(CEntitySAInterface* pEntity)
{

    CObjectSAInterface* pInterface = static_cast<CObjectSAInterface*>(pEntity);

    SClientEntity<CObjectSA>* pObjectClientEntity = g_pCore->GetGame()->GetPools()->GetObjectA((DWORD*)pInterface);
    CObjectSA* pObject = pObjectClientEntity ? pObjectClientEntity->pEntity : nullptr;
    if (pObject != nullptr)
    {
        bGotReplaced = true;
        pColModelReplace = GetScaledCollision(pObject);
    }
    else
    {
        bGotReplaced = false;
    }
}

void _declspec(naked) HOOK_GetBoundRect()
{
    _asm
    {
        pushad
        push    esi
        call    GetBoundRect
        add     esp, 4
        popad

        mov     ecx, pColModelReplace;
        jmp CONTINUE_CWorld_GetBoundRect;
    }
}

    void _declspec(naked) HOOK_GetColModel()
    {
        _asm
        {
        pushad
        push    ecx
        call    GetColModel
        mov     colModel, eax
        add     esp, 4
        popad
        }

        if (bIsObject)
        {
            colModel = DoSomethingWithCollision(pObject);
            _asm
            {
            mov eax, colModel
            retn
            }
        }
        else
        {
            _asm {
        mov al, [ecx + 36h]
        and al, 7 
        jmp CONTINUE_CWorld_GetColModel
            }
        }
    }

    void _declspec(naked) HOOK_FallenPeds()
    {
        if (pGame && pGame->IsUnderWorldWarpEnabled())
        {
            _asm {
            sub esp, 2Ch
            push ebx
            mov ebx, ds:0B74490h
            jmp CONTINUE_CWorld_FallenPeds
            }
        }
        else
        {
            _asm
            {
            ret
            }
        }
    }

void _declspec(naked) HOOK_FallenCars()
{
    if (pGame && pGame->IsUnderWorldWarpEnabled())
    {
        _asm
            sub esp, 2Ch
            push ebx
            mov ebx, ds:0B74494h
            jmp CONTINUE_CWorld_FallenCars
            }
    }
    else
    {
        _asm
        {
            ret
        }
    }
}

void CWorldSA::Add(CEntity* pEntity, eDebugCaller CallerId)
{
    DEBUG_TRACE("VOID CWorldSA::Add ( CEntity * pEntity )");

    CEntitySA* pEntitySA = dynamic_cast<CEntitySA*>(pEntity);

    if (pEntitySA)
    {
        CEntitySAInterface* pInterface = pEntitySA->GetInterface();
        if ((DWORD)pInterface->vtbl == VTBL_CPlaceable)
        {
            SString strMessage("Caller: %i ", CallerId);
            LogEvent(506, "CWorld::Add ( CEntity * ) Crash", "", strMessage);
        }
        DWORD dwEntity = (DWORD)pEntitySA->GetInterface();
        DWORD dwFunction = FUNC_Add;
        _asm
        {
            push    dwEntity
            call    dwFunction
            add     esp, 4
        }
    }
}

void CWorldSA::Add(CEntitySAInterface* entityInterface, eDebugCaller CallerId)
{
    DEBUG_TRACE("VOID CWorldSA::Add ( CEntitySAInterface * entityInterface )");
    DWORD dwFunction = FUNC_Add;
    if ((DWORD)entityInterface->vtbl == VTBL_CPlaceable)
    {
        SString strMessage("Caller: %i ", CallerId);
        LogEvent(506, "CWorld::Add ( CEntitySAInterface * ) Crash", "", strMessage);
    }
    _asm
    {
        push    entityInterface
        call    dwFunction
        add     esp, 4
    }
}

void CWorldSA::Remove(CEntity* pEntity, eDebugCaller CallerId)
{
    DEBUG_TRACE("VOID CWorldSA::Remove ( CEntity * entity )");

    CEntitySA* pEntitySA = dynamic_cast<CEntitySA*>(pEntity);

    if (pEntitySA)
    {
        CEntitySAInterface* pInterface = pEntitySA->GetInterface();
        if ((DWORD)pInterface->vtbl == VTBL_CPlaceable)
        {
            SString strMessage("Caller: %i ", CallerId);
            LogEvent(507, "CWorld::Remove ( CEntity * ) Crash", "", strMessage);
        }
        DWORD dwEntity = (DWORD)pInterface;
        DWORD dwFunction = FUNC_Remove;
        _asm
        {
            push    dwEntity
            call    dwFunction
            add     esp, 4
        }
    }
}

void CWorldSA::Remove(CEntitySAInterface* entityInterface, eDebugCaller CallerId)
{
    DEBUG_TRACE("VOID CWorldSA::Remove ( CEntitySAInterface * entityInterface )");
    if ((DWORD)entityInterface->vtbl == VTBL_CPlaceable)
    {
        SString strMessage("Caller: %i ", CallerId);
        LogEvent(507, "CWorld::Remove ( CEntitySAInterface * ) Crash", "", strMessage);
    }
    DWORD dwFunction = FUNC_Remove;
    _asm
    {
        push    entityInterface
        call    dwFunction
        add     esp, 4

    /*  mov     ecx, entityInterface
        mov     esi, [ecx]
        push    1
        call    dword ptr [esi+8]*/
    }
}

void CWorldSA::RemoveReferencesToDeletedObject(CEntitySAInterface* entity)
{
    DWORD dwFunc = FUNC_RemoveReferencesToDeletedObject;
    DWORD dwEntity = (DWORD)entity;
    _asm
    {
        push    dwEntity
        call    dwFunc
        add     esp, 4
    }
}

// THIS FUNCTION IS INCOMPLETE AND SHOULD NOT BE USED
bool CWorldSA::TestLineSphere(CVector* vecStart, CVector* vecEnd, CVector* vecSphereCenter, float fSphereRadius, CColPoint** colCollision)
{
    // Create a CColLine for us
    DWORD dwFunc = FUNC_CColLine_Constructor;
    DWORD dwCColLine[10];            // I don't know how big CColLine is, so we'll just be safe
    _asm
    {
        lea     ecx, dwCColLine
        push    vecEnd
        push    vecStart
        call    dwFunc
    }

    // Now, lets make a CColSphere
    BYTE byteColSphere[18];            // looks like its 18 bytes { vecPos, fSize, byteUnk, byteUnk, byteUnk }
    dwFunc = FUNC_CColSphere_Set;
    _asm
    {
        lea     ecx, byteColSphere
        push    255
        push    0
        push    0
        push    vecSphereCenter
        push    fSphereRadius
        call    dwFunc
    }
}

void ConvertMatrixToEulerAngles(const CMatrix_Padded& matrixPadded, float& fX, float& fY, float& fZ)
{
    // Convert the given matrix to a padded matrix
    // CMatrix_Padded matrixPadded ( Matrix );

    // Grab its pointer and call gta's func
    const CMatrix_Padded* pMatrixPadded = &matrixPadded;
    DWORD                 dwFunc = FUNC_CMatrix__ConvertToEulerAngles;

    float* pfX = &fX;
    float* pfY = &fY;
    float* pfZ = &fZ;
    int    iUnknown = 21;
    _asm
    {
        push    iUnknown
            push    pfZ
            push    pfY
            push    pfX
            mov     ecx, pMatrixPadded
            call    dwFunc
    }
}

bool CWorldSA::ProcessLineOfSight(const CVector* vecStart, const CVector* vecEnd, CColPoint** colCollision, CEntity** CollisionEntity,
                                  const SLineOfSightFlags flags, SLineOfSightBuildingResult* pBuildingResult)
{
    DEBUG_TRACE("VOID CWorldSA::ProcessLineOfSight(CVector * vecStart, CVector * vecEnd, CColPoint * colCollision, CEntity * CollisionEntity)");
    DWORD dwPadding[100];            // stops the function missbehaving and overwriting the return address
    dwPadding[0] = 0;                // prevent the warning and eventual compiler optimizations from removing it

    CColPointSA*          pColPointSA = new CColPointSA();
    CColPointSAInterface* pColPointSAInterface = pColPointSA->GetInterface();

    // DWORD targetEntity;
    CEntitySAInterface* targetEntity = NULL;
    bool                bReturn = false;

    DWORD dwFunc = FUNC_ProcessLineOfSight;
    // bool bCheckBuildings = true,                 bool bCheckVehicles = true,     bool bCheckPeds = true,
    // bool bCheckObjects = true,                   bool bCheckDummies = true,      bool bSeeThroughStuff = false,
    // bool bIgnoreSomeObjectsForCamera = false,    bool bShootThroughStuff = false
    MemPutFast<BYTE>(VAR_CWorld_bIncludeCarTires, flags.bCheckCarTires);

    _asm
    {
        push    flags.bShootThroughStuff
        push    flags.bIgnoreSomeObjectsForCamera
        push    flags.bSeeThroughStuff
        push    flags.bCheckDummies
        push    flags.bCheckObjects
        push    flags.bCheckPeds
        push    flags.bCheckVehicles
        push    flags.bCheckBuildings
        lea     eax, targetEntity
        push    eax
        push    pColPointSAInterface
        push    vecEnd
        push    vecStart
        call    dwFunc
        mov     bReturn, al
        add     esp, 0x30
    }

    MemPutFast<BYTE>(VAR_CWorld_bIncludeCarTires, 0);

    // Building info needed?
    if (pBuildingResult)
    {
        CPoolsSA* pPools = ((CPoolsSA*)pGame->GetPools());
        if (pPools)
        {
            if (targetEntity && targetEntity->nType == ENTITY_TYPE_BUILDING)
            {
                pBuildingResult->bValid = true;
                pBuildingResult->usModelID = targetEntity->m_nModelIndex;
                if (targetEntity->m_pLod)
                    pBuildingResult->usLODModelID = targetEntity->m_pLod->m_nModelIndex;
                else
                    pBuildingResult->usLODModelID = 0;

                pBuildingResult->pInterface = targetEntity;
                pBuildingResult->vecPosition = targetEntity->Placeable.m_transform.m_translate;
                if (targetEntity->Placeable.matrix)
                {
                    CVector& vecRotation = pBuildingResult->vecRotation;
                    ConvertMatrixToEulerAngles(*targetEntity->Placeable.matrix, vecRotation.fX, vecRotation.fY, vecRotation.fZ);
                    vecRotation = -vecRotation;
                }
            }
            if (targetEntity && targetEntity->nType == ENTITY_TYPE_OBJECT)
            {
                pBuildingResult->bValid = true;
                pBuildingResult->usModelID = targetEntity->m_nModelIndex;
                if (targetEntity->m_pLod)
                    pBuildingResult->usLODModelID = targetEntity->m_pLod->m_nModelIndex;
                else
                    pBuildingResult->usLODModelID = 0;

                pBuildingResult->pInterface = targetEntity;
            }
        }
    }

    if (CollisionEntity)
    {
        CPools* pPools = pGame->GetPools();
        if (pPools)
        {
            if (targetEntity)
            {
                *CollisionEntity = pPools->GetEntity((DWORD*)targetEntity);
            }
        }
    }
    if (colCollision)
        *colCollision = pColPointSA;
    else
        pColPointSA->Destroy();

    return bReturn;
}

void CWorldSA::IgnoreEntity(CEntity* pEntity)
{
    DEBUG_TRACE("VOID CWorldSA::IgnoreEntity(CEntity * entity)");

    CEntitySA* pEntitySA = dynamic_cast<CEntitySA*>(pEntity);

    if (pEntitySA)
        MemPutFast<DWORD>(VAR_IgnoredEntity, (DWORD)pEntitySA->GetInterface());
    else
        MemPutFast<DWORD>(VAR_IgnoredEntity, 0);
}

// technically this is in CTheZones
BYTE CWorldSA::GetLevelFromPosition(CVector* vecPosition)
{
    DEBUG_TRACE("BYTE CWorldSA::GetLevelFromPosition(CVector * vecPosition)");
    DWORD dwFunc = FUNC_GetLevelFromPosition;
    BYTE  bReturn = 0;
    _asm
    {
        push    vecPosition
        call    dwFunc
        mov     bReturn, al
        pop     eax
    }
    return bReturn;
}

float CWorldSA::FindGroundZForPosition(float fX, float fY)
{
    DEBUG_TRACE("FLOAT CWorldSA::FindGroundZForPosition(FLOAT fX, FLOAT fY)");
    DWORD dwFunc = FUNC_FindGroundZFor3DCoord;
    FLOAT fReturn = 0;
    _asm
    {
        push    fY
        push    fX
        call    dwFunc
        fstp    fReturn
        add     esp, 8
    }
    return fReturn;
}

float CWorldSA::FindGroundZFor3DPosition(CVector* vecPosition)
{
    DEBUG_TRACE("FLOAT CWorldSA::FindGroundZFor3DPosition(CVector * vecPosition)");
    DWORD dwFunc = FUNC_FindGroundZFor3DCoord;
    FLOAT fReturn = 0;
    FLOAT fX = vecPosition->fX;
    FLOAT fY = vecPosition->fY;
    FLOAT fZ = vecPosition->fZ;
    _asm
    {
        push    0
        push    0
        push    fZ
        push    fY
        push    fX
        call    dwFunc
        fstp    fReturn
        add     esp, 0x14
    }
    return fReturn;
}

void CWorldSA::LoadMapAroundPoint(CVector* vecPosition, FLOAT fRadius)
{
    DEBUG_TRACE("VOID CWorldSA::LoadMapAroundPoint(CVector * vecPosition, FLOAT fRadius)");
    DWORD dwFunc = FUNC_CTimer_Stop;
    _asm
    {
        call    dwFunc
    }

    dwFunc = FUNC_CRenderer_RequestObjectsInDirection;
    _asm
    {
        push    32
        push    fRadius
        push    vecPosition
        call    dwFunc
        add     esp, 12
    }

    dwFunc = FUNC_CStreaming_LoadScene;
    _asm
    {
        push    vecPosition
        call    dwFunc
        add     esp, 4
    }

    dwFunc = FUNC_CTimer_Update;
    _asm
    {
        call    dwFunc
    }
}

bool CWorldSA::IsLineOfSightClear(const CVector* vecStart, const CVector* vecEnd, const SLineOfSightFlags flags)
{
    DWORD dwFunc = FUNC_IsLineOfSightClear;
    bool  bReturn = false;
    // bool bCheckBuildings = true, bool bCheckVehicles = true, bool bCheckPeds = true,
    // bool bCheckObjects = true, bool bCheckDummies = true, bool bSeeThroughStuff = false,
    // bool bIgnoreSomeObjectsForCamera = false

    _asm
    {
        push    flags.bIgnoreSomeObjectsForCamera
        push    flags.bSeeThroughStuff
        push    flags.bCheckDummies
        push    flags.bCheckObjects
        push    flags.bCheckPeds
        push    flags.bCheckVehicles
        push    flags.bCheckBuildings
        push    vecEnd
        push    vecStart
        call    dwFunc
        mov     bReturn, al
        add     esp, 0x24
    }
    return bReturn;
}

bool CWorldSA::HasCollisionBeenLoaded(CVector* vecPosition)
{
    DWORD dwFunc = FUNC_HasCollisionBeenLoaded;
    bool  bRet = false;
    _asm
    {
        push    0
        push    vecPosition
        call    dwFunc
        mov     bRet, al
        add     esp, 8
    }
    return bRet;
}

DWORD CWorldSA::GetCurrentArea()
{
    return *(DWORD*)VAR_currArea;
}

void CWorldSA::SetCurrentArea(DWORD dwArea)
{
    MemPutFast<DWORD>(VAR_currArea, dwArea);

    DWORD dwFunc = FUNC_RemoveBuildingsNotInArea;
    _asm
    {
        push    dwArea
        call    dwFunc
        add     esp, 4
    }
}

void CWorldSA::SetJetpackMaxHeight(float fHeight)
{
    MemPut<float>(VAR_fJetpackMaxHeight, fHeight);
}

float CWorldSA::GetJetpackMaxHeight()
{
    return *(float*)(VAR_fJetpackMaxHeight);
}

void CWorldSA::SetAircraftMaxHeight(float fHeight)
{
    g_pCore->GetMultiplayer()->SetAircraftMaxHeight(fHeight);
}

float CWorldSA::GetAircraftMaxHeight()
{
    return g_pCore->GetMultiplayer()->GetAircraftMaxHeight();
}

void CWorldSA::SetAircraftMaxVelocity(float fVelocity)
{
    g_pCore->GetMultiplayer()->SetAircraftMaxVelocity(fVelocity);
}

float CWorldSA::GetAircraftMaxVelocity()
{
    return g_pCore->GetMultiplayer()->GetAircraftMaxVelocity();
}

void CWorldSA::SetOcclusionsEnabled(bool bEnabled)
{
    if (!bEnabled)
    {
        MemPut<BYTE>(FUNC_COcclusion_ProcessBeforeRendering, 0xC3);            // retn
        MemPutFast<int>(VAR_COcclusion_NumActiveOccluders, 0);
        MemCpy((void*)CALL_CCullZones_FindTunnelAttributesForCoors, "\xB8\x80\x28\x00\x00", 5);            // mov eax, 0x2880
    }
    else
    {
        MemPut<BYTE>(FUNC_COcclusion_ProcessBeforeRendering, 0x51);                                        // Standard value
        MemCpy((void*)CALL_CCullZones_FindTunnelAttributesForCoors, "\xE8\xDE\x82\x1D\x00", 5);            // call 0x72D9F0
    }
}

bool CWorldSA::GetOcclusionsEnabled()
{
    if (*(BYTE*)FUNC_COcclusion_ProcessBeforeRendering == 0x51)            // Is standard value ?
        return true;
    return false;
}

void CWorldSA::FindWorldPositionForRailTrackPosition(float fRailTrackPosition, int iTrackId, CVector* pOutVecPosition)
{
    DWORD dwFunc = FUNC_CWorld_FindPositionForTrackPosition;            // __cdecl

    _asm
    {
        push pOutVecPosition
        push iTrackId
        push fRailTrackPosition
        call dwFunc
        add  esp, 3*4
    }
}

int CWorldSA::FindClosestRailTrackNode(const CVector& vecPosition, uchar& ucOutTrackId, float& fOutRailDistance)
{
    // Original function @ 0x6F7550
    int           iNodeId = -1;
    float         fMinDistance = 99999.898f;
    int*          aNumTrackNodes = (int*)ARRAY_NumRailTrackNodes;
    SRailNodeSA** aTrackNodes = (SRailNodeSA**)ARRAY_RailTrackNodePointers;
    uchar         ucDesiredTrackId = ucOutTrackId;

    for (uchar ucTrackId = 0; ucTrackId < NUM_RAILTRACKS; ++ucTrackId)
    {
        if ((ucDesiredTrackId == 0xFF || ucTrackId == ucDesiredTrackId) && aNumTrackNodes[ucTrackId] > 0)
        {
            for (int i = 0; i < aNumTrackNodes[ucTrackId]; ++i)
            {
                SRailNodeSA& railNode = aTrackNodes[ucTrackId][i];

                float fDistance = sqrt(pow(vecPosition.fZ - railNode.sZ * 0.125f, 2) + pow(vecPosition.fY - railNode.sY * 0.125f, 2) +
                                       pow(vecPosition.fX - railNode.sX * 0.125f, 2));
                if (fDistance < fMinDistance)
                {
                    fMinDistance = fDistance;
                    iNodeId = i;
                    ucOutTrackId = ucTrackId;
                }
            }
        }
    }

    // Read rail distance
    fOutRailDistance = aTrackNodes[ucOutTrackId][iNodeId].sRailDistance * 3.33333334f;

    return iNodeId;

    /*DWORD dwFunc = FUNC_GetTrainNodeNearPoint; // __cdecl
    int iNodeId;
    *pOutTrackId = 0;
    float fX = vecPosition.fX;
    float fY = vecPosition.fY;
    float fZ = vecPosition.fZ;

    _asm
    {
        push pOutTrackId
        push fZ
        push fY
        push fZ
        call dwFunc
        mov iNodeId, eax
        add esp, 4*4
    }

    return iNodeId;*/
}

void CWorldSA::RemoveBuilding(unsigned short usModelToRemove, float fRange, float fX, float fY, float fZ, char cInterior, uint* pOutAmount)
{
    // New building Removal
    SBuildingRemoval* pRemoval = new SBuildingRemoval();
    pRemoval->m_usModel = usModelToRemove;
    pRemoval->m_vecPos.fX = fX;
    pRemoval->m_vecPos.fY = fY;
    pRemoval->m_vecPos.fZ = fZ;
    pRemoval->m_fRadius = fRange;
    pRemoval->m_cInterior = cInterior;
    // Push it to the back of the removal list
    m_pBuildingRemovals->insert(std::pair<unsigned short, SBuildingRemoval*>(usModelToRemove, pRemoval));

    bool bFound = false;
    uint uiAmount = 0;
    // Init loop variables
    std::pair<std::multimap<unsigned short, sDataBuildingRemovalItem*>::iterator, std::multimap<unsigned short, sDataBuildingRemovalItem*>::iterator>
                                                                             iterators = m_pDataBuildings->equal_range(usModelToRemove);
    std::multimap<unsigned short, sDataBuildingRemovalItem*>::const_iterator iter = iterators.first;
    for (; iter != iterators.second; ++iter)
    {
        sDataBuildingRemovalItem* pFind = (*iter).second;
        if (pFind)
        {
            // if the count is <= 0 and the interface is valid check the distance in case we found a removal (count is used to store if we have already removed
            // this once)
            if (pFind->m_iCount <= 0 && pFind->m_pInterface)
            {
                // Grab distances across each axis
                float fDistanceX = fX - pFind->m_pInterface->Placeable.m_transform.m_translate.fX;
                float fDistanceY = fY - pFind->m_pInterface->Placeable.m_transform.m_translate.fY;
                float fDistanceZ = fZ - pFind->m_pInterface->Placeable.m_transform.m_translate.fZ;

                if (pFind->m_pInterface->Placeable.matrix != NULL)
                {
                    fDistanceX = fX - pFind->m_pInterface->Placeable.matrix->vPos.fX;
                    fDistanceY = fY - pFind->m_pInterface->Placeable.matrix->vPos.fY;
                    fDistanceZ = fZ - pFind->m_pInterface->Placeable.matrix->vPos.fZ;
                }

                // Square root 'em
                float fDistance = sqrt(fDistanceX * fDistanceX + fDistanceY * fDistanceY + fDistanceZ * fDistanceZ);

                // Is it in range
                if (fDistance <= fRange && (pFind->m_pInterface->m_areaCode == cInterior || cInterior == -1))
                {
                    CEntitySAInterface* pInterface = pFind->m_pInterface;
                    // while ( pInterface && pInterface != NULL )
                    // if the interface is valid
                    if (pInterface && pInterface != NULL)
                    {
                        // if the building type is dummy or building and it's not already being removed
                        if ((pInterface->nType == ENTITY_TYPE_BUILDING || pInterface->nType == ENTITY_TYPE_DUMMY || pInterface->nType == ENTITY_TYPE_OBJECT) &&
                            pInterface->bRemoveFromWorld != 1)
                        {
                            if ((DWORD)(pInterface->vtbl) != VTBL_CPlaceable)
                            {
                                // Add the Data Building to the list
                                pRemoval->AddDataBuilding(pInterface);
                                // Remove the model from the world
                                Remove(pInterface, BuildingRemoval2);
                                m_pRemovedEntities[(DWORD)pInterface] = true;
                                bFound = true;
                            }
                        }
                        // Get next LOD ( LOD's can have LOD's so we keep checking pInterface )
                        // pInterface = pInterface->m_pLod;
                    }
                    // Set the count.
                    pFind->m_iCount = 1;
                }
            }
        }
    }

    std::pair<std::multimap<unsigned short, sBuildingRemovalItem*>::iterator, std::multimap<unsigned short, sBuildingRemovalItem*>::iterator> iteratorsBinary =
        m_pBinaryBuildings->equal_range(usModelToRemove);
    std::multimap<unsigned short, sBuildingRemovalItem*>::const_iterator iterBinary = iteratorsBinary.first;
    for (; iterBinary != iteratorsBinary.second; ++iterBinary)
    {
        sBuildingRemovalItem* pFindBinary = (*iterBinary).second;
        if (pFindBinary)
        {
            // if the count is <= 0 and the interface is valid check the distance in case we found a removal (count is used to store if we have already removed
            // this once)
            if (pFindBinary->m_iCount <= 0 && pFindBinary->m_pInterface)
            {
                // Grab distances across each axis
                float fDistanceX = fX - pFindBinary->m_pInterface->Placeable.m_transform.m_translate.fX;
                float fDistanceY = fY - pFindBinary->m_pInterface->Placeable.m_transform.m_translate.fY;
                float fDistanceZ = fZ - pFindBinary->m_pInterface->Placeable.m_transform.m_translate.fZ;

                if (pFindBinary->m_pInterface->Placeable.matrix != NULL)
                {
                    fDistanceX = fX - pFindBinary->m_pInterface->Placeable.matrix->vPos.fX;
                    fDistanceY = fY - pFindBinary->m_pInterface->Placeable.matrix->vPos.fY;
                    fDistanceZ = fZ - pFindBinary->m_pInterface->Placeable.matrix->vPos.fZ;
                }

                // Square root 'em
                float fDistance = sqrt(fDistanceX * fDistanceX + fDistanceY * fDistanceY + fDistanceZ * fDistanceZ);

                // Is it in range
                if (fDistance <= fRange && (pFindBinary->m_pInterface->m_areaCode == cInterior || cInterior == -1))
                {
                    CEntitySAInterface* pInterface = pFindBinary->m_pInterface;
                    // while ( pInterface && pInterface != NULL )
                    // if the interface is valid
                    if (pInterface && pInterface != NULL)
                    {
                        // if the building type is dummy or building and it's not already being removed
                        if ((pInterface->nType == ENTITY_TYPE_BUILDING || pInterface->nType == ENTITY_TYPE_DUMMY || pInterface->nType == ENTITY_TYPE_OBJECT) &&
                            pInterface->bRemoveFromWorld != 1)
                        {
                            if ((DWORD)(pInterface->vtbl) != VTBL_CPlaceable)
                            {
                                // Add the Data Building to the list
                                pRemoval->AddBinaryBuilding(pInterface);
                                // Remove the model from the world
                                Remove(pInterface, BuildingRemoval2);
                                m_pRemovedEntities[(DWORD)pInterface] = true;
                                bFound = true;
                                ++uiAmount;
                            }
                        }
                        // Get next LOD ( LOD's can have LOD's so we keep checking pInterface )
                        // pInterface = pInterface->m_pLod;
                    }
                    // Set the count.
                    pFindBinary->m_iCount = 1;
                }
            }
        }
    }
    if (bFound)
        pGame->GetModelInfo(usModelToRemove)->RestreamIPL();

    if (pOutAmount)
        *pOutAmount = uiAmount;
}

bool CWorldSA::RestoreBuilding(unsigned short usModelToRestore, float fRange, float fX, float fY, float fZ, char cInterior, uint* pOutAmount)
{
    bool bSuccess = false;
    uint uiAmount = 0;

    // Init some variables
    std::pair<std::multimap<unsigned short, SBuildingRemoval*>::iterator, std::multimap<unsigned short, SBuildingRemoval*>::iterator> iterators =
        m_pBuildingRemovals->equal_range(usModelToRestore);
    std::multimap<unsigned short, SBuildingRemoval*>::const_iterator iter = iterators.first;
    // Loop through the buildings list
    for (; iter != iterators.second;)
    {
        SBuildingRemoval* pFind = (*iter).second;
        // if pFind is valid and the model is the same
        if (pFind)
        {
            // Grab distances across each axis
            float fDistanceX = fX - pFind->m_vecPos.fX;
            float fDistanceY = fY - pFind->m_vecPos.fY;
            float fDistanceZ = fZ - pFind->m_vecPos.fZ;

            // Square root 'em
            float fDistance = sqrt(fDistanceX * fDistanceX + fDistanceY * fDistanceY + fDistanceZ * fDistanceZ);
            if (fDistance <= pFind->m_fRadius && (cInterior == -1 || pFind->m_cInterior == cInterior))
            {
                // Init some variables
                CEntitySAInterface*                            pEntity = NULL;
                std::list<CEntitySAInterface*>::const_iterator entityIter = pFind->m_pBinaryRemoveList->begin();
                if (pFind->m_pBinaryRemoveList->empty() == false)
                {
                    // Loop through the Binary object list
                    for (; entityIter != pFind->m_pBinaryRemoveList->end();)
                    {
                        // Grab the pEntity
                        pEntity = (*entityIter);
                        // if it's valid re-add it to the world.
                        if (pEntity != NULL)
                        {
                            // Remove it from the binary list
                            pFind->m_pBinaryRemoveList->erase(entityIter++);
                            // if the building type is dummy or building and it's not already being removed
                            if ((pEntity->nType == ENTITY_TYPE_BUILDING || pEntity->nType == ENTITY_TYPE_DUMMY || pEntity->nType == ENTITY_TYPE_OBJECT) &&
                                pEntity->bRemoveFromWorld != 1)
                            {
                                // Don't call this on entities being removed.
                                if ((DWORD)(pEntity->vtbl) != VTBL_CPlaceable)
                                {
                                    Add(pEntity, Building_Restore);
                                    m_pRemovedEntities[(DWORD)pEntity] = false;
                                    // If the building isn't streamed in, we won't find the building in the list (because the hook wasn't called) ->
                                    // removeWorldModel doesn't work next time
                                    AddBinaryBuilding(pEntity);
                                    ++uiAmount;
                                }
                            }
                        }
                        else
                            ++entityIter;
                    }
                }
                // Start the iterator with the data remove list first item
                entityIter = pFind->m_pDataRemoveList->begin();
                if (pFind->m_pDataRemoveList->empty() == false)
                {
                    // Loop through the Data list
                    for (; entityIter != pFind->m_pDataRemoveList->end();)
                    {
                        // Grab the pEntity
                        pEntity = (*entityIter);
                        // if it's valid re-add it to the world.
                        if (pEntity != NULL)
                        {
                            pFind->m_pDataRemoveList->erase(entityIter++);
                            // if the building type is dummy or building and it's not already being removed
                            if ((pEntity->nType == ENTITY_TYPE_BUILDING || pEntity->nType == ENTITY_TYPE_DUMMY || pEntity->nType == ENTITY_TYPE_OBJECT) &&
                                pEntity->bRemoveFromWorld != 1)
                            {
                                if ((DWORD)(pEntity->vtbl) != VTBL_CPlaceable)
                                {
                                    Add(pEntity, Building_Restore2);
                                    m_pRemovedEntities[(DWORD)pEntity] = false;
                                }
                            }
                        }
                        else
                        {
                            ++entityIter;
                        }
                    }
                }
                // Remove the building from the list
                m_pBuildingRemovals->erase(iter++);
                delete pFind;
                // Success! don't return incase there are any others to delete
                bSuccess = true;
            }
            else
                iter++;
        }
        else
            iter++;
    }
    std::pair<std::multimap<unsigned short, sDataBuildingRemovalItem*>::iterator, std::multimap<unsigned short, sDataBuildingRemovalItem*>::iterator>
                                                                             dataBuildingIterators = m_pDataBuildings->equal_range(usModelToRestore);
    std::multimap<unsigned short, sDataBuildingRemovalItem*>::const_iterator iterator = dataBuildingIterators.first;
    for (; iterator != dataBuildingIterators.second; ++iterator)
    {
        sDataBuildingRemovalItem* pFound = (*iterator).second;
        if (pFound)
        {
            // Grab distances across each axis
            float fDistanceX = fX - pFound->m_pInterface->Placeable.m_transform.m_translate.fX;
            float fDistanceY = fY - pFound->m_pInterface->Placeable.m_transform.m_translate.fY;
            float fDistanceZ = fZ - pFound->m_pInterface->Placeable.m_transform.m_translate.fZ;

            if (pFound->m_pInterface->Placeable.matrix != NULL)
            {
                fDistanceX = fX - pFound->m_pInterface->Placeable.matrix->vPos.fX;
                fDistanceY = fY - pFound->m_pInterface->Placeable.matrix->vPos.fY;
                fDistanceZ = fZ - pFound->m_pInterface->Placeable.matrix->vPos.fZ;
            }

            // Square root 'em
            float fDistance = sqrt(fDistanceX * fDistanceX + fDistanceY * fDistanceY + fDistanceZ * fDistanceZ);
            if (fDistance <= fRange && (cInterior == -1 || pFound->m_pInterface->m_areaCode == cInterior))
            {
                // Fix the removed count.
                pFound->m_iCount--;
                if (pFound->m_iCount < 0)
                    pFound->m_iCount = 0;
            }
        }
    }

    std::pair<std::multimap<unsigned short, sBuildingRemovalItem*>::iterator, std::multimap<unsigned short, sBuildingRemovalItem*>::iterator>
                                                                         binaryBuildingIterators = m_pBinaryBuildings->equal_range(usModelToRestore);
    std::multimap<unsigned short, sBuildingRemovalItem*>::const_iterator iteratorBinary = binaryBuildingIterators.first;
    for (; iteratorBinary != binaryBuildingIterators.second; ++iteratorBinary)
    {
        sBuildingRemovalItem* pFoundBinary = (*iteratorBinary).second;
        if (pFoundBinary)
        {
            // Grab distances across each axis
            float fDistanceX = fX - pFoundBinary->m_pInterface->Placeable.m_transform.m_translate.fX;
            float fDistanceY = fY - pFoundBinary->m_pInterface->Placeable.m_transform.m_translate.fY;
            float fDistanceZ = fZ - pFoundBinary->m_pInterface->Placeable.m_transform.m_translate.fZ;

            if (pFoundBinary->m_pInterface->Placeable.matrix != NULL)
            {
                fDistanceX = fX - pFoundBinary->m_pInterface->Placeable.matrix->vPos.fX;
                fDistanceY = fY - pFoundBinary->m_pInterface->Placeable.matrix->vPos.fY;
                fDistanceZ = fZ - pFoundBinary->m_pInterface->Placeable.matrix->vPos.fZ;
            }

            // Square root 'em
            float fDistance = sqrt(fDistanceX * fDistanceX + fDistanceY * fDistanceY + fDistanceZ * fDistanceZ);
            if (fDistance <= fRange && (cInterior == -1 || pFoundBinary->m_pInterface->m_areaCode == cInterior))
            {
                // Fix the removed count.
                pFoundBinary->m_iCount--;
                if (pFoundBinary->m_iCount < 0)
                    pFoundBinary->m_iCount = 0;
            }
        }
    }

    if (pOutAmount)
        *pOutAmount = uiAmount;

    return bSuccess;
}

// Check Distance to see if the model being requested is in the radius
bool CWorldSA::IsRemovedModelInRadius(SIPLInst* pInst)
{
    // Init some variables
    std::pair<std::multimap<unsigned short, SBuildingRemoval*>::iterator, std::multimap<unsigned short, SBuildingRemoval*>::iterator> iterators =
        m_pBuildingRemovals->equal_range(pInst->m_nModelIndex);
    std::multimap<unsigned short, SBuildingRemoval*>::const_iterator iter = iterators.first;
    // Loop through the buildings list
    for (; iter != iterators.second; ++iter)
    {
        SBuildingRemoval* pFind = (*iter).second;
        // if pFind is valid and the model is the same
        if (pFind)
        {
            // Grab the distance
            float fDistanceX = pFind->m_vecPos.fX - pInst->m_pPosition.fX;
            float fDistanceY = pFind->m_vecPos.fY - pInst->m_pPosition.fY;
            float fDistanceZ = pFind->m_vecPos.fZ - pInst->m_pPosition.fZ;

            float fDistance = sqrt(fDistanceX * fDistanceX + fDistanceY * fDistanceY + fDistanceZ * fDistanceZ);
            // is it in the removal spheres radius if so return else keep looking
            if (fDistance <= pFind->m_fRadius && (pFind->m_cInterior == -1 || pFind->m_cInterior == pInst->m_nInterior))
            {
                return true;
            }
        }
    }
    return false;
}

// Check Distance to see if the model being requested is in the radius
bool CWorldSA::IsObjectRemoved(CEntitySAInterface* pInterface)
{
    // Init some variables
    std::pair<std::multimap<unsigned short, SBuildingRemoval*>::iterator, std::multimap<unsigned short, SBuildingRemoval*>::iterator> iterators =
        m_pBuildingRemovals->equal_range(pInterface->m_nModelIndex);
    std::multimap<unsigned short, SBuildingRemoval*>::const_iterator iter = iterators.first;
    // Loop through the buildings list
    for (; iter != iterators.second; ++iter)
    {
        SBuildingRemoval* pFind = (*iter).second;
        // if pFind is valid and the model is the same
        if (pFind)
        {
            // Are we using the interior param?? if so check for a match
            if (pFind->m_cInterior == -1 || pFind->m_cInterior == pInterface->m_areaCode)
            {
                // Grab the distance
                float fDistanceX = pFind->m_vecPos.fX - pInterface->Placeable.m_transform.m_translate.fX;
                float fDistanceY = pFind->m_vecPos.fY - pInterface->Placeable.m_transform.m_translate.fY;
                float fDistanceZ = pFind->m_vecPos.fZ - pInterface->Placeable.m_transform.m_translate.fZ;

                if (pInterface->Placeable.matrix != NULL)
                {
                    fDistanceX = pFind->m_vecPos.fX - pInterface->Placeable.matrix->vPos.fX;
                    fDistanceY = pFind->m_vecPos.fY - pInterface->Placeable.matrix->vPos.fY;
                    fDistanceZ = pFind->m_vecPos.fZ - pInterface->Placeable.matrix->vPos.fZ;
                }

                float fDistance = sqrt(fDistanceX * fDistanceX + fDistanceY * fDistanceY + fDistanceZ * fDistanceZ);
                // is it in the removal spheres radius if so return else keep looking
                if (fDistance <= pFind->m_fRadius)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

// Check if a given model is replaced
bool CWorldSA::IsModelRemoved(unsigned short usModelID)
{
    return m_pBuildingRemovals->count(usModelID) > 0;
}

// Check if a given model is replaced
bool CWorldSA::IsDataModelRemoved(unsigned short usModelID)
{
    return m_pBuildingRemovals->count(usModelID) > 0;
}

// Check if a given model is replaced
bool CWorldSA::IsEntityRemoved(CEntitySAInterface* pInterface)
{
    return m_pRemovedEntities.find((DWORD)pInterface) != m_pRemovedEntities.end() && m_pRemovedEntities[(DWORD)pInterface] == true;
}

// Resets deleted list
void CWorldSA::ClearRemovedBuildingLists(uint* pOutAmount)
{
    // Ensure no memory leaks by deleting items.
    uint                                                             uiAmount = 0;
    std::multimap<unsigned short, SBuildingRemoval*>::const_iterator iter = m_pBuildingRemovals->begin();

    for (; iter != m_pBuildingRemovals->end();)
    {
        SBuildingRemoval* pFind = (*iter).second;
        if (pFind)
        {
            // Init some variables
            CEntitySAInterface*                            pEntity = NULL;
            std::list<CEntitySAInterface*>::const_iterator entityIter = pFind->m_pBinaryRemoveList->begin();
            if (pFind->m_pBinaryRemoveList->empty() == false)
            {
                // Loop through the Binary remove list
                for (; entityIter != pFind->m_pBinaryRemoveList->end(); ++entityIter)
                {
                    // Grab the pEntity
                    pEntity = (*entityIter);
                    // if it's valid re-add it to the world.
                    if (pEntity && pEntity != NULL)
                    {
                        // if the building type is dummy or building and it's not already being removed
                        if ((pEntity->nType == ENTITY_TYPE_BUILDING || pEntity->nType == ENTITY_TYPE_DUMMY || pEntity->nType == ENTITY_TYPE_OBJECT) &&
                            pEntity->bRemoveFromWorld != 1)
                        {
                            // Don't call this on entities being removed.
                            if ((DWORD)(pEntity->vtbl) != VTBL_CPlaceable)
                            {
                                Add(pEntity, BuildingRemovalReset);
                                m_pRemovedEntities[(DWORD)pEntity] = false;
                                ++uiAmount;
                            }
                        }
                    }
                }
            }
            entityIter = pFind->m_pDataRemoveList->begin();
            if (pFind->m_pDataRemoveList->empty() == false)
            {
                // Loop through the Data list
                for (; entityIter != pFind->m_pDataRemoveList->end(); ++entityIter)
                {
                    // Grab the pEntity
                    pEntity = (*entityIter);
                    // if it's valid re-add it to the world.
                    if (pEntity && pEntity != NULL)
                    {
                        // if the building type is dummy or building and it's not already being removed
                        if ((pEntity->nType == ENTITY_TYPE_BUILDING || pEntity->nType == ENTITY_TYPE_DUMMY || pEntity->nType == ENTITY_TYPE_OBJECT) &&
                            pEntity->bRemoveFromWorld != 1)
                        {
                            // Don't call this on entities being removed.
                            if ((DWORD)(pEntity->vtbl) != VTBL_CPlaceable)
                            {
                                Add(pEntity, BuildingRemovalReset2);
                                m_pRemovedEntities[(DWORD)pEntity] = false;
                            }
                        }
                    }
                }
            }
            m_pBuildingRemovals->erase(iter++);
        }
        else
            iter++;
    }
    // Init some variables
    std::multimap<unsigned short, sDataBuildingRemovalItem*>::const_iterator iterator = m_pDataBuildings->begin();
    // Loop through the data building list
    for (; iterator != m_pDataBuildings->end(); ++iterator)
    {
        sDataBuildingRemovalItem* pFound = (*iterator).second;
        if (pFound)
        {
            // Set the count to 0 so we can remove it again
            pFound->m_iCount = 0;
        }
    }
    // Init some variables
    std::multimap<unsigned short, sBuildingRemovalItem*>::const_iterator iteratorBinary = m_pBinaryBuildings->begin();
    // Loop through the data building list
    for (; iteratorBinary != m_pBinaryBuildings->end(); ++iteratorBinary)
    {
        sBuildingRemovalItem* pFoundBinary = (*iteratorBinary).second;
        if (pFoundBinary)
        {
            // Set the count to 0 so we can remove it again
            pFoundBinary->m_iCount = 0;
        }
    }
    // Delete old building lists
    delete m_pBuildingRemovals;
    // Create new
    m_pBuildingRemovals = new std::multimap<unsigned short, SBuildingRemoval*>;
    m_pRemovedEntities.clear();

    if (pOutAmount)
        *pOutAmount = uiAmount;
}

// Resets deleted list
SBuildingRemoval* CWorldSA::GetBuildingRemoval(CEntitySAInterface* pInterface)
{
    // Init some variables
    std::pair<std::multimap<unsigned short, SBuildingRemoval*>::iterator, std::multimap<unsigned short, SBuildingRemoval*>::iterator> iterators =
        m_pBuildingRemovals->equal_range(pInterface->m_nModelIndex);
    std::multimap<unsigned short, SBuildingRemoval*>::const_iterator iter = iterators.first;
    // Loop through the buildings list
    for (; iter != iterators.second; ++iter)
    {
        SBuildingRemoval* pFind = (*iter).second;
        // if pFind is valid and the model is the same
        if (pFind)
        {
            // Grab the distance
            float fDistanceX = pFind->m_vecPos.fX - pInterface->Placeable.m_transform.m_translate.fX;
            float fDistanceY = pFind->m_vecPos.fY - pInterface->Placeable.m_transform.m_translate.fY;
            float fDistanceZ = pFind->m_vecPos.fZ - pInterface->Placeable.m_transform.m_translate.fZ;

            if (pInterface->Placeable.matrix != NULL)
            {
                fDistanceX = pFind->m_vecPos.fX - pInterface->Placeable.matrix->vPos.fX;
                fDistanceY = pFind->m_vecPos.fY - pInterface->Placeable.matrix->vPos.fY;
                fDistanceZ = pFind->m_vecPos.fZ - pInterface->Placeable.matrix->vPos.fZ;
            }

            float fDistance = sqrt(fDistanceX * fDistanceX + fDistanceY * fDistanceY + fDistanceZ * fDistanceZ);
            // is it in the removal spheres radius if so return else keep looking
            if (fDistance <= pFind->m_fRadius && (pFind->m_cInterior == -1 || pFind->m_cInterior == pInterface->m_areaCode))
            {
                return pFind;
            }
        }
    }
    return NULL;
}

void CWorldSA::AddDataBuilding(CEntitySAInterface* pInterface)
{
    if (m_pAddedEntities.find((DWORD)pInterface) == m_pAddedEntities.end() || m_pAddedEntities[(DWORD)pInterface] == false)
    {
        // Create a new building removal
        sDataBuildingRemovalItem* pBuildingRemoval = new sDataBuildingRemovalItem(pInterface, true);
        // Insert it with the model index so we can fast lookup
        m_pDataBuildings->insert(std::pair<unsigned short, sDataBuildingRemovalItem*>((unsigned short)pInterface->m_nModelIndex, pBuildingRemoval));
        m_pAddedEntities[(DWORD)pInterface] = true;
        m_pRemovedEntities[(DWORD)pInterface] = false;
    }
}

void CWorldSA::AddBinaryBuilding(CEntitySAInterface* pInterface)
{
    if (m_pAddedEntities.find((DWORD)pInterface) == m_pAddedEntities.end() || m_pAddedEntities[(DWORD)pInterface] == false)
    {
        // Create a new building removal
        sBuildingRemovalItem* pBuildingRemoval = new sBuildingRemovalItem(pInterface, false);
        // Insert it with the model index so we can fast lookup
        m_pBinaryBuildings->insert(std::pair<unsigned short, sBuildingRemovalItem*>((unsigned short)pInterface->m_nModelIndex, pBuildingRemoval));
        m_pAddedEntities[(DWORD)pInterface] = true;
        m_pRemovedEntities[(DWORD)pInterface] = false;
    }
}

void CWorldSA::RemoveWorldBuildingFromLists(CEntitySAInterface* pInterface)
{
    std::pair<std::multimap<unsigned short, SBuildingRemoval*>::iterator, std::multimap<unsigned short, SBuildingRemoval*>::iterator> iterators =
        m_pBuildingRemovals->equal_range(pInterface->m_nModelIndex);
    std::multimap<unsigned short, SBuildingRemoval*>::const_iterator iter = iterators.first;

    // Loop through the buildings list
    for (; iter != iterators.second; ++iter)
    {
        SBuildingRemoval* pFind = (*iter).second;
        // if pFind is valid and the model is the same
        if (pFind)
        {
            CEntitySAInterface* pEntity = NULL;
            // if the binary remove list is empty don't continue
            if (pFind->m_pBinaryRemoveList->empty() == false)
            {
                // grab the beginning
                std::list<CEntitySAInterface*>::const_iterator entityIter = pFind->m_pBinaryRemoveList->begin();
                // Loop through the binary remove list
                for (; entityIter != pFind->m_pBinaryRemoveList->end();)
                {
                    pEntity = (*entityIter);
                    // is the pointer the same as the one being deleted
                    if ((DWORD)pEntity == (DWORD)pInterface)
                    {
                        // remove it from the binary removed list for this removal
                        pFind->m_pBinaryRemoveList->erase(entityIter++);
                    }
                    else
                        entityIter++;
                }
            }
            if (pFind->m_pDataRemoveList->empty() == false)
            {
                std::list<CEntitySAInterface*>::const_iterator entityIter = pFind->m_pDataRemoveList->begin();
                // Loop through the Data list
                for (; entityIter != pFind->m_pDataRemoveList->end();)
                {
                    // Grab the pEntity
                    pEntity = (*entityIter);
                    // is the pointer the same as the one being deleted
                    if ((DWORD)pEntity == (DWORD)pInterface)
                    {
                        // remove it from the data removed list for this removal
                        pFind->m_pDataRemoveList->erase(entityIter++);
                    }
                    else
                        entityIter++;
                }
            }
        }
    }
    {
        // Init some variables
        std::pair<std::multimap<unsigned short, sDataBuildingRemovalItem*>::iterator, std::multimap<unsigned short, sDataBuildingRemovalItem*>::iterator>
                                                                                 dataIterators = m_pDataBuildings->equal_range(pInterface->m_nModelIndex);
        std::multimap<unsigned short, sDataBuildingRemovalItem*>::const_iterator iterator = dataIterators.first;
        for (; iterator != dataIterators.second;)
        {
            sDataBuildingRemovalItem* pFound = (*iterator).second;
            if (pFound)
            {
                // is the pointer the same as the one being deleted
                if ((DWORD)pFound->m_pInterface == (DWORD)pInterface)
                {
                    // remove it from the data buildings list so we don't try and remove or add it again.
                    m_pDataBuildings->erase(iterator++);
                }
                else
                    iterator++;
            }
            else
                iterator++;
        }
    }
    {
        // Init some variables
        std::pair<std::multimap<unsigned short, sBuildingRemovalItem*>::iterator, std::multimap<unsigned short, sBuildingRemovalItem*>::iterator>
                                                                             binaryIterators = m_pBinaryBuildings->equal_range(pInterface->m_nModelIndex);
        std::multimap<unsigned short, sBuildingRemovalItem*>::const_iterator iteratorBinary = binaryIterators.first;
        for (; iteratorBinary != binaryIterators.second;)
        {
            sBuildingRemovalItem* pFound = (*iteratorBinary).second;
            if (pFound)
            {
                // is the pointer the same as the one being deleted
                if ((DWORD)pFound->m_pInterface == (DWORD)pInterface)
                {
                    // remove it from the data buildings list so we don't try and remove or add it again.
                    m_pBinaryBuildings->erase(iteratorBinary++);
                }
                else
                    iteratorBinary++;
            }
            else
                iteratorBinary++;
        }
    }
    m_pRemovedEntities[(DWORD)pInterface] = false;
    m_pAddedEntities[(DWORD)pInterface] = false;
}

bool CWorldSA::CalculateImpactPosition(const CVector& vecInputStart, CVector& vecInputEnd)
{
    // get our position
    CVector vecStart = vecInputStart;
    // get our end position by projecting forward a few velocities more
    CVector vecEnd = vecInputEnd;
    // grab the difference between our reported and actual end position
    CVector diff = vecEnd - vecStart;
    // normalize our difference
    diff.Normalize();
    // project forward another unit
    vecEnd = vecEnd + diff * 1;
    // create a variable to store our collision data
    CColPoint* pColPoint;
    // create a variable to store our collision entity
    CEntity* pCollisionEntity;

    // flags
    SLineOfSightFlags flags;
    flags.bCheckCarTires = false;
    flags.bIgnoreSomeObjectsForCamera = true;
    flags.bCheckBuildings = true;
    flags.bCheckPeds = true;
    flags.bCheckObjects = true;
    flags.bCheckDummies = true;

    // Include dead peds
    MemPutFast<DWORD>(0xB7CD71, 1);

    SLineOfSightBuildingResult result;
    // process forward another 1 unit
    if (ProcessLineOfSight(&vecStart, &vecEnd, &pColPoint, &pCollisionEntity, flags, &result))
    {
        // set our collision position
        vecInputEnd = pColPoint->GetPosition();

        // destroy our colshape
        pColPoint->Destroy();

        // reset include dead peds
        MemPutFast<DWORD>(0xB7CD71, 0);
        return true;
    }
    // Include dead peds
    MemPutFast<DWORD>(0xB7CD71, 0);
    return false;
}

namespace
{
    const unsigned char aOriginalSurfaceInfo[2292] = {
        0x00, 0x00, 0xc0, 0x40, 0x66, 0x66, 0x66, 0x40, 0x00, 0x00, 0x90, 0x40, 0xcd, 0xcc, 0x4c, 0x40,
        0x00, 0x00, 0x40, 0x40, 0x33, 0x33, 0x33, 0x40, 0x66, 0x66, 0x66, 0x40, 0x00, 0x00, 0x00, 0x40,
        0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x60, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x00, 0x40,
        0x00, 0x00, 0x90, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0xc0, 0x40, 0x00, 0x00, 0x00, 0x40,
        0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3f, 0xcd, 0xcc, 0x4c, 0x40, 0x00, 0x00, 0x60, 0x40,
        0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x80, 0x3f,
        0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3f,
        0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x80, 0x3f, 0x33, 0x33, 0x33, 0x40, 0x00, 0x00, 0x00, 0x40,
        0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0x3f,
        0x0a, 0xe7, 0x00, 0x00, 0x22, 0x01, 0x00, 0x00, 0x40, 0x06, 0x00, 0x00, 0x0a, 0xe7, 0x00, 0x00,
        0x22, 0x01, 0x20, 0x48, 0x40, 0x06, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x22, 0x01, 0x80, 0x08,
        0x40, 0x06, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x22, 0x01, 0x00, 0x09, 0xc0, 0x06, 0x00, 0x00,
        0x0a, 0xe7, 0x00, 0x00, 0x22, 0x01, 0x60, 0x48, 0x00, 0x06, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x22, 0x01, 0x00, 0x08, 0x00, 0x06, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x23, 0x09, 0x80, 0xc1,
        0x04, 0x22, 0x00, 0x00, 0x0a, 0xe7, 0x00, 0x00, 0x22, 0x01, 0x80, 0x48, 0x00, 0x06, 0x00, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x22, 0x01, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x0a, 0xd8, 0x00, 0x00,
        0x0b, 0x0a, 0x80, 0x02, 0x82, 0x0a, 0x00, 0x00, 0x0a, 0xd8, 0x00, 0x00, 0x0b, 0x0a, 0x80, 0x02,
        0x82, 0x0a, 0x00, 0x00, 0x0a, 0xd8, 0x00, 0x00, 0x0b, 0x0a, 0x80, 0x82, 0x82, 0x02, 0x02, 0x00,
        0x0a, 0xd8, 0x00, 0x00, 0x0b, 0x0a, 0x80, 0x02, 0x82, 0x0a, 0x00, 0x00, 0x0a, 0xd8, 0x00, 0x00,
        0x0b, 0x0a, 0x80, 0x02, 0x82, 0x0a, 0x00, 0x00, 0x0a, 0xd8, 0x00, 0x00, 0x0b, 0x0a, 0x80, 0x82,
        0x82, 0x02, 0x02, 0x00, 0x0a, 0xd8, 0x00, 0x00, 0x0b, 0x0a, 0x80, 0x02, 0x02, 0x0a, 0x00, 0x00,
        0x0a, 0xd8, 0x00, 0x00, 0x0b, 0x0a, 0x80, 0x02, 0x02, 0x0a, 0x00, 0x00, 0x0a, 0x28, 0x00, 0x00,
        0x03, 0x0a, 0x04, 0x02, 0x00, 0x08, 0x00, 0x00, 0x0a, 0xd8, 0x00, 0x00, 0x23, 0x01, 0x84, 0x01,
        0x00, 0x04, 0x00, 0x00, 0x0a, 0xd8, 0x00, 0x00, 0x03, 0x0a, 0x80, 0x00, 0x00, 0x0a, 0x00, 0x00,
        0x0a, 0xd8, 0x00, 0x00, 0x03, 0x0a, 0x80, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x02, 0x02, 0x00, 0x81, 0x00, 0x06, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x21, 0x02, 0x00, 0x81,
        0x80, 0x06, 0x00, 0x00, 0x0a, 0xd8, 0x00, 0x00, 0x03, 0x0b, 0x00, 0x01, 0x00, 0x0a, 0x00, 0x00,
        0x0a, 0xe7, 0x00, 0x00, 0x0c, 0x0a, 0x00, 0x81, 0x08, 0x02, 0x00, 0x00, 0x0a, 0xe7, 0x00, 0x00,
        0x29, 0x02, 0x00, 0x81, 0x08, 0x22, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x02, 0x02, 0x80, 0x80,
        0x10, 0x22, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x02, 0x02, 0x00, 0xc0, 0x10, 0x22, 0x00, 0x00,
        0x0a, 0x32, 0x00, 0x00, 0x14, 0x4a, 0x00, 0xa0, 0xa0, 0x12, 0x00, 0x00, 0x0a, 0x32, 0x00, 0x00,
        0x14, 0x4a, 0x00, 0xa0, 0xa0, 0x12, 0x00, 0x00, 0x0a, 0x32, 0x00, 0x00, 0x12, 0x0a, 0x00, 0xa0,
        0xa0, 0x12, 0x00, 0x00, 0x0a, 0x32, 0x00, 0x00, 0x14, 0x4a, 0x00, 0xa0, 0xa0, 0x12, 0x00, 0x00,
        0x0a, 0x32, 0x00, 0x00, 0x14, 0x4a, 0x00, 0xa0, 0xa0, 0x12, 0x00, 0x00, 0x0a, 0x32, 0x00, 0x00,
        0x14, 0x4a, 0x02, 0xa0, 0x20, 0x12, 0x00, 0x00, 0x0a, 0xe7, 0x00, 0x00, 0x02, 0x01, 0x02, 0x48,
        0x00, 0x12, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x21, 0x01, 0x00, 0x49, 0x00, 0x06, 0x00, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x21, 0x01, 0x00, 0x01, 0x00, 0x06, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x21, 0x01, 0x00, 0x01, 0x00, 0x06, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x05, 0x8a, 0x00, 0x01,
        0x01, 0x82, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x05, 0x8a, 0x01, 0x01, 0x01, 0x82, 0x00, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x03, 0x0a, 0x80, 0x80, 0x80, 0x02, 0x02, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x03, 0x0a, 0x80, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x0a, 0xe7, 0x00, 0x00, 0x02, 0x03, 0x00, 0x02,
        0x00, 0x42, 0x00, 0x00, 0x0a, 0xe7, 0x00, 0x00, 0x02, 0x03, 0x00, 0x02, 0x00, 0x42, 0x00, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x02, 0x03, 0x00, 0x02, 0x00, 0x42, 0x00, 0x00, 0x0a, 0xd8, 0x00, 0x00,
        0x21, 0x11, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x21, 0x11, 0x08, 0x00,
        0x00, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x21, 0x11, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x21, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x21, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x0a, 0xd8, 0x00, 0x00, 0x21, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x01, 0x00, 0x0a, 0xd8, 0x00, 0x00, 0x21, 0x01, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00,
        0x0a, 0xd8, 0x00, 0x00, 0x21, 0x31, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0a, 0xd8, 0x00, 0x00,
        0x21, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0a, 0xd8, 0x00, 0x00, 0x21, 0x01, 0x00, 0x00,
        0x00, 0x02, 0x01, 0x00, 0x0a, 0xd8, 0x00, 0x00, 0x21, 0x31, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00,
        0x0a, 0xd8, 0x00, 0x00, 0x21, 0x01, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x0a, 0xd8, 0x00, 0x00,
        0x21, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x0a, 0xd8, 0x00, 0x00, 0x21, 0x01, 0x00, 0x00,
        0x00, 0x02, 0x01, 0x00, 0x0a, 0xd8, 0x00, 0x00, 0x21, 0x01, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00,
        0x0a, 0xd8, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0a, 0xe7, 0x00, 0x00,
        0x03, 0x03, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x0a, 0xd8, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
        0x00, 0x04, 0x00, 0x00, 0x0a, 0xd8, 0x00, 0x00, 0x21, 0x01, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00,
        0x0a, 0xd8, 0x00, 0x00, 0x21, 0x01, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x21, 0x01, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x0a, 0xe7, 0x00, 0x00, 0x21, 0x31, 0x00, 0x00,
        0x00, 0x06, 0x00, 0x00, 0x0a, 0xd8, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00,
        0x0a, 0xd8, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x0a, 0xe7, 0x00, 0x00,
        0x21, 0x11, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x0a, 0xe7, 0x00, 0x00, 0x02, 0x03, 0x00, 0x00,
        0x00, 0x40, 0x00, 0x00, 0x0a, 0xe7, 0x00, 0x00, 0x21, 0x0c, 0x00, 0x10, 0x00, 0x12, 0x00, 0x00,
        0x0a, 0xe7, 0x00, 0x00, 0x21, 0x03, 0x00, 0x10, 0x00, 0x42, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x02, 0x03, 0x10, 0x00, 0x00, 0x42, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x14, 0x42, 0x00, 0xa0,
        0xa0, 0x13, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x14, 0x42, 0x80, 0xa0, 0xa0, 0x13, 0x00, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x14, 0x42, 0x00, 0xa0, 0xa0, 0x13, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x14, 0x42, 0x00, 0xa0, 0xa0, 0x13, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x14, 0x42, 0x00, 0xa0,
        0x20, 0x13, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x14, 0x42, 0x02, 0xa0, 0x20, 0x13, 0x00, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x0b, 0x04, 0x80, 0x00, 0x82, 0x0b, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x0b, 0x04, 0x80, 0x00, 0x82, 0x0b, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0b, 0x04, 0x80, 0x00,
        0x82, 0x0b, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x03, 0x04, 0x80, 0x00, 0x88, 0x07, 0x00, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x03, 0x04, 0x80, 0x00, 0x88, 0x07, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x23, 0x04, 0x80, 0x00, 0x08, 0x07, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x23, 0x04, 0x80, 0x00,
        0x84, 0x07, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x03, 0x04, 0x00, 0x00, 0x08, 0x03, 0x02, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x01, 0x04, 0x00, 0x00, 0x08, 0x07, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x22, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00,
        0x00, 0x43, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x43, 0x00, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x43, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x02, 0x01, 0x00, 0x00, 0x00, 0x43, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00,
        0x00, 0x43, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x02, 0x03, 0x00, 0x00, 0x00, 0x43, 0x00, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x12, 0xca, 0x00, 0x00, 0x21, 0x83, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x12, 0xca, 0x00, 0x00, 0x21, 0x83, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x12, 0xca, 0x00, 0x00,
        0x21, 0x83, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x12, 0xca, 0x00, 0x00, 0x21, 0x83, 0x00, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x12, 0xca, 0x00, 0x00, 0x21, 0x83, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x02, 0x01, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00,
        0x00, 0x43, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x03, 0x04, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x43, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x02, 0x01, 0x00, 0x00, 0x00, 0x43, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00,
        0x00, 0x03, 0x04, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x03, 0x04, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x43, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x23, 0x01, 0x00, 0x80, 0x00, 0x23, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x80,
        0x00, 0x0b, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x80, 0x02, 0x03, 0x02, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x80, 0x02, 0x03, 0x02, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x03, 0x01, 0x00, 0x80, 0x82, 0x03, 0x02, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x80,
        0x02, 0x03, 0x02, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0b, 0x04, 0x80, 0x80, 0x82, 0x0b, 0x00, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x0b, 0x04, 0x80, 0x80, 0x82, 0x03, 0x02, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x0b, 0x04, 0x80, 0x80, 0x82, 0x03, 0x02, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0b, 0x04, 0x80, 0x80,
        0x82, 0x0b, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0b, 0x04, 0x80, 0x80, 0x82, 0x0b, 0x00, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x0b, 0x04, 0x80, 0x80, 0x82, 0x0b, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x0b, 0x04, 0x80, 0x80, 0x82, 0x0b, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0b, 0x04, 0x80, 0x80,
        0x82, 0x0b, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0b, 0x04, 0x80, 0x80, 0x08, 0x13, 0x00, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x03, 0x04, 0x80, 0x80, 0x00, 0x13, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x03, 0x04, 0x00, 0x80, 0x08, 0x0b, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x03, 0x04, 0x00, 0x00,
        0x01, 0x13, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x02, 0x04, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x0b, 0x04, 0x80, 0x80, 0x88, 0x13, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x0b, 0x04, 0x00, 0x81, 0x08, 0x13, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0b, 0x04, 0x80, 0x80,
        0x08, 0x13, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x13, 0x04, 0x80, 0x80, 0x00, 0x13, 0x00, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x0b, 0x04, 0x80, 0x80, 0x08, 0x13, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x02, 0x02, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00,
        0x00, 0x13, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x02, 0x01, 0x00, 0x00, 0x00, 0x43, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x22, 0x01, 0x00, 0x00,
        0x00, 0x07, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x02, 0x01, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00,
        0x00, 0x23, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x0a, 0xd8, 0x00, 0x00,
        0x03, 0x0a, 0x00, 0x80, 0x00, 0x03, 0x02, 0x00, 0x0a, 0xd8, 0x00, 0x00, 0x0b, 0x0c, 0x80, 0x80,
        0x80, 0x0b, 0x00, 0x00, 0x0a, 0xd8, 0x00, 0x00, 0x0b, 0x0c, 0x80, 0x80, 0x80, 0x0b, 0x00, 0x00,
        0x0a, 0xd8, 0x00, 0x00, 0x0b, 0x0c, 0x80, 0x80, 0x80, 0x0b, 0x00, 0x00, 0x0a, 0xd8, 0x00, 0x00,
        0x0b, 0x0c, 0x80, 0x80, 0x80, 0x0b, 0x00, 0x00, 0x0a, 0xd8, 0x00, 0x00, 0x0b, 0x0c, 0x80, 0x80,
        0x80, 0x0b, 0x00, 0x00, 0x0a, 0xd8, 0x00, 0x00, 0x0b, 0x0c, 0x80, 0x00, 0x80, 0x0b, 0x00, 0x00,
        0x0a, 0xd8, 0x00, 0x00, 0x0b, 0x0c, 0x80, 0x00, 0x80, 0x0b, 0x00, 0x00, 0x0a, 0xd8, 0x00, 0x00,
        0x0b, 0x0c, 0x80, 0x00, 0x80, 0x0b, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00,
        0x00, 0x83, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x02, 0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x0a, 0xe7, 0x00, 0x00, 0x02, 0x03, 0x00, 0x00,
        0x00, 0x40, 0x00, 0x00, 0x0a, 0xe7, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00,
        0x0a, 0xe7, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x02, 0x01, 0x10, 0x00, 0x00, 0x06, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x02, 0x01, 0x10, 0x00,
        0x00, 0x02, 0x01, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x02, 0x03, 0x10, 0x00, 0x00, 0x42, 0x00, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x22, 0x01, 0x10, 0x10, 0x00, 0x02, 0x01, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x22, 0x01, 0x10, 0x10, 0x00, 0x06, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x02, 0x04, 0x10, 0x10,
        0x00, 0x10, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x22, 0x01, 0x10, 0x10, 0x00, 0x02, 0x01, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x22, 0x01, 0x10, 0x10, 0x00, 0x00, 0x01, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x02, 0x04, 0x10, 0x10, 0x00, 0x10, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x02, 0x01, 0x10, 0x10,
        0x00, 0x10, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x22, 0x01, 0x10, 0x10, 0x00, 0x02, 0x01, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x02, 0x03, 0x10, 0x10, 0x00, 0x42, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x02, 0x03, 0x10, 0x10, 0x00, 0x42, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x02, 0x03, 0x10, 0x10,
        0x00, 0x42, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x22, 0x01, 0x10, 0x10, 0x00, 0x00, 0x04, 0x00,
        0x0a, 0x00, 0x00, 0x00, 0x02, 0x04, 0x10, 0x10, 0x00, 0x0a, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x02, 0x01, 0x10, 0x10, 0x00, 0x12, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x02, 0x01, 0x90, 0x01,
        0x00, 0x22, 0x00, 0x00
    };
    static_assert(sizeof(CSurfaceType) == sizeof(aOriginalSurfaceInfo), "invalid size of aOriginalSurfaceInfo");
}
