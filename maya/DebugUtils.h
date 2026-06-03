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

    inline void createDebugLocator(const std::string& name, const MVector& point, const MVector& up_vector, const double scale)
    {
        /*
        print in the output window the python command to create a locator at the specified point..
        */
        char debug_locator_msg[2048];
        sprintf_s(debug_locator_msg,

    "scale = %f\n\
loc = cmds.spaceLocator(n=\"%s\")[0]\n\
cmds.setAttr(\"{}.localScaleX\".format(loc), scale)\n\
cmds.setAttr(\"{}.localScaleY\".format(loc), scale)\n\
cmds.setAttr(\"{}.localScaleZ\".format(loc), scale)\n\
loc_up = cmds.spaceLocator(n=\"%s_UP\")[0]\n\
cmds.setAttr(\"{}.localScaleX\".format(loc_up), scale)\n\
cmds.setAttr(\"{}.localScaleY\".format(loc_up), scale)\n\
cmds.setAttr(\"{}.localScaleZ\".format(loc_up), scale)\n\
cmds.xform(loc_up, t=(%f, %f, %f))\n\
cmds.parent(loc_up, loc)\n\
cmds.xform(loc, t=(%f, %f, %f))\n\n", scale,
                                          name.c_str(),
                                          name.c_str(),
                                          up_vector[0],
                                          up_vector[1],
                                          up_vector[2],
                                          point[0],
                                          point[1],
                                          point[2]);

        MGlobal::executePythonCommandOnIdle(debug_locator_msg);
    }
}
#endif //STEELFACETOOLS_DEBUGUTILS_H
