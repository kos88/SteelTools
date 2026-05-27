//
// Created by Costantino Fracas on 27/05/2026.
//

#ifndef STEELFACETOOLS_DEBUGUTILS_H
#define STEELFACETOOLS_DEBUGUTILS_H
#include <vector>

#include <maya/MGlobal.h>
#include <maya/MVector.h>

namespace DebugUtils
{
    // Dump a curve to visualize data
    inline void createDebugCurve(const char* name, const std::vector<MVector>& points, const bool update)
    {
        // Build the points list string
        std::string ptList;
        for (unsigned int i = 0; i < points.size(); ++i)
        {
            char pt[128];
            sprintf_s(pt, "(%.6f, %.6f, %.6f)", points[i][0], points[i][1], points[i][2]);
            ptList += pt;
            if (i < points.size() - 1) ptList += ", ";
        }

        // Build cv update string
        std::string cvUpdates;
        for (unsigned int i = 0; i < points.size(); ++i)
        {
            char cv[256];
            sprintf_s(cv, "    cmds.setAttr('%sShape.cv[%d]', %.6f, %.6f, %.6f, type='double3')\n",
                      name, i, points[i][0], points[i][1], points[i][2]);
            cvUpdates += cv;
        }

        std::string cmd;
        if (update)
        {
            cmd += "if not cmds.objExists('" + std::string(name) + "'):\n";
            cmd += "    t_name = cmds.curve(n='" + std::string(name) + "', p=[" + ptList + "], d=1)\n";
            cmd += "    shape = cmds.listRelatives(t_name, shapes=True)[0]\n";
            cmd += "    cmds.setAttr(f\"{shape}.overrideEnabled\", 1)\n";
            cmd += "    cmds.setAttr(f\"{shape}.overrideColor\", 13)\n";
            cmd += "    cmds.rename(shape, f\"{t_name}Shape\")\n";
            cmd += "else:\n";
            cmd += cvUpdates;
        }
        else
        {
            cmd += "cmds.curve(n='" + std::string(name) + "', p=[" + ptList + "], d=1)\n";
        }

        MGlobal::executePythonCommandOnIdle(cmd.c_str());
    }
}
#endif //STEELFACETOOLS_DEBUGUTILS_H
