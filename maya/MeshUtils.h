//
// Created by Costantino Fracas on 22/06/2026.
//

#ifndef STEELFACETOOLS_MESHUTILS_H
#define STEELFACETOOLS_MESHUTILS_H
#include <unordered_map>

#include <maya/MGlobal.h>
#include <maya/MItMeshEdge.h>
#include <maya/MFnMesh.h>
#include <maya/MObject.h>


namespace SteelMeshUtils
{

    struct Neighbours
    {
        int a = -1;
        int b = -1;
    };

    /**
     * Reorder the input edges through connectivity
     * @param edgeObj MObject containing the edges that are part of a loop
     * @param meshObj MObject containing the mesh where these edges are from
     * @return a pair with the ordered points and the ordered indexes
     */
    inline std::pair<std::vector<MVector>, std::vector<int>> createOrderedVertices(MObject& edgeObj, MObject& meshObj)
    {
        std::unordered_map<int, Neighbours> adj;
        const MFnMesh fnMesh(meshObj);

        MItMeshEdge edgeIter(meshObj, edgeObj);
        for (; !edgeIter.isDone(); edgeIter.next())
        {
            int v0 = edgeIter.index(0);
            int v1 = edgeIter.index(1);

            auto& vertexA_adj = adj[v0];

            if (vertexA_adj.a == -1)
                vertexA_adj.a = v1;
            else
                vertexA_adj.b = v1;

            auto& vertexB_adj = adj[v1];

            if (vertexB_adj.a == -1)
                vertexB_adj.a = v0;
            else
                vertexB_adj.b = v0;
        }

        // Find start: endpoint has only one neighbour
        int start = adj.begin()->first;
        for (auto& [v, n] : adj)
            if (n.b == -1) { start = v; break; }

        // Walk
        std::vector<MVector> orderedPts;
        std::vector<int> originalIndexes;
        orderedPts.reserve(adj.size());

        int prev = -1;
        int cur  = start;
        while (cur != -1)
        {
            MPoint pt;
            fnMesh.getPoint(cur, pt, MSpace::kObject);

            // soa output
            orderedPts.emplace_back(pt);
            originalIndexes.push_back(cur);

            auto [nA, nB] = adj[cur];
            int next = (nA != prev) ? nA : nB;
            if (next == start) next = -1;
            prev = cur;
            cur  = next;
        }

        return std::make_pair(orderedPts, originalIndexes);

    };
}

#endif //STEELFACETOOLS_MESHUTILS_H
