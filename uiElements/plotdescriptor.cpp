/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#include "plotdescriptor.h"

PLOT_TYPE strToPlotTypeId(QString typeString)
{
    if (typeString == "plot") return PTJK_PLOT;
    if (typeString == "scatter")   return PTJK_SCATTER;
    if (typeString == "area")  return PTJK_AREA;
    if (typeString == "barv")  return PTJK_BARV;
    if (typeString == "dens") return PTJK_DENS;
    if (typeString == "vartarrows")  return PTJK_VERTARROWS;
    if (typeString == "boxplot") return PTJK_BOXPLOT;
    if (typeString == "vect2d")   return PTJK_VECT2D;
    if (typeString == "contour") return PTJK_CONTOUR;
    return PTJK_PLOT;
}



QString plotTypeIdToString(PLOT_TYPE rpt)
{
    if (rpt==PTJK_PLOT)    return "plot";
    if (rpt==PTJK_SCATTER)      return "scatter";
    if (rpt==PTJK_AREA)     return "area";
    if (rpt==PTJK_BARV)     return "barv";
    if (rpt==PTJK_DENS)    return "dens";
    if (rpt==PTJK_VERTARROWS)     return "vertarrows";
    if (rpt==PTJK_BOXPLOT)    return "boxplot";
    if (rpt==PTJK_VECT2D)      return "vect2d";
    if (rpt==PTJK_CONTOUR)    return "contour";
    return "undefined";
}
