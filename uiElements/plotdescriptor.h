/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#ifndef PLOTDESCRIPTOR_H
#define PLOTDESCRIPTOR_H
#include <vector>
enum PLOT_TYPE{PTJK_PLOT, PTJK_SCATTER, PTJK_AREA, PTJK_BARV, PTJK_DENS, PTJK_VERTARROWS, PTJK_BOXPLOT, PTJK_VECT2D, PTJK_CONTOUR,
               PTGL_MESH, PTGL_SURF, PTGL_CONT3D, PTGL_DENSSLICE,PTGL_VECT3D };

#include <string>
#include <QString>
struct plot_descriptor
{
    PLOT_TYPE       _ptype;
    std::string     _indep_x;        // it is used as ax for vect
    std::string     _indep_y;        // not used for 2D plot, it is used as ay for vect plots
    std::string     _indep_z;        // not used for 2D plot, it is used as az for vect plots
    std::string     _dep;
    std::string     _depy;          // used for vector data
    std::string     _options;
    std::vector<void*> _graph;      // Link to graph(s) (more than one if variable is 2D)
    int             _plot_id;       // Plot id (e.g. subplotting)
    bool            _init_zoom;
    plot_descriptor() : _graph() {}
};


PLOT_TYPE strToPlotTypeId(QString typeString);

QString plotTypeIdToString(PLOT_TYPE rpt);


#endif // PLOTDESCRIPTOR_H
