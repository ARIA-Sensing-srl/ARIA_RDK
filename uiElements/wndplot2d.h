/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#ifndef WNDPLOT2D_H
#define WNDPLOT2D_H

#include <QDialog>
#include "jkqtplotter.h"
#include "plotdescriptor.h"
#include "octavews.h"
#include "qgridlayout.h"


namespace Ui {
class wndPlot2d;
}

class wndPlot2d : public QDialog
{
    Q_OBJECT

public:
    explicit wndPlot2d(class   mdiOctaveInterface* parent = nullptr, octavews* ws=nullptr);
    explicit wndPlot2d(class   MainWindow*         parent = nullptr, octavews* ws=nullptr);
    ~wndPlot2d();
    QGridLayout* _layout;
    octavews*   get_workspace();
    void        remove_workspace();
    // 2D Plot
    int         add_plot(const QString& var, const QString& x=QString(""), int plot_id=-1);
    int         add_scatterplot(const QString& var, const QString& x=QString(""), int plot_id=-1);
    int         add_barplot(const QString& var, const QString& x=QString(""), int plot_id=-1);
    int         add_areaplot(const QString& var, const QString& x=QString(""), int plot_id=-1);
    int         add_arrowplot(const QString& var, const QString& x=QString(""), int plot_id=-1);
    int         add_boxplot(const QString& var, const QString& x=QString(""), int plot_id=-1);
    int         add_vector_plot(const QString& var_x, const QString& var_y, const QString& x=QString(""),const QString& y=QString(""), int plot_id=-1);
    int         add_density_plot(const QString& var_x, const QString& x=QString(""),const QString& y=QString(""), int plot_id=-1);
    int         add_contour_plot(const QString& var_x, const QString& x=QString(""),const QString& y=QString(""), int plot_id=-1);
    void        update_workspace(octavews* ws);
    void        clear_owners();
    QString     last_error() {return _last_error;}

private:
    Ui::wndPlot2d *ui;

    void* graph;
    JKQTPGraph*                        create_graph(plot_descriptor& pl, JKQTPlotter* plotter);
    octavews*                          _workspace;
    int                                _nplot_x,_nplot_y, _total_plot;
    std::vector<plot_descriptor>       _plots;
    std::vector<JKQTPlotter*>          _plotters;   // List of widgets

    QString                            _last_error;
    QString                            get_var_name(std::string basename, int order);
    int                                get_var_count(std::string varname);
    int                                get_var_length(std::string varname);
    QString                            get_var_indep_name(const std::string& indep, const std::string& dep, size_t dim=0);
    QString                            get_vector_full_name(const std::string& vector_x, const std::string& vector_y);

    void        create_plots();
    void        update_plots();
    void        clear_plots();
    void        update_graphs_per_dep(plot_descriptor& pl, JKQTPlotter* plotter);
    void        update_grahs_per_dep_boxplot(plot_descriptor& pl, JKQTPlotter* plotter);
    void        update_graphs_vector(plot_descriptor& pl, JKQTPlotter* plotter);
    void        update_graphs_density(plot_descriptor& pl, JKQTPlotter* plotter);
    void        octave_to_plotdata(std::string varname);
    void        octave_to_box_plotdata(std::string varname, std::string indep);
    void        octave_to_vectordata(std::string varname_x, std::string varname_y, std::string xname="", std::string yname="");
    void        octave_to_densitydata(std::string varname_x, std::string xname="", std::string yname="")  ;
    void        default_indep_vect(const std::string& indep, const std::string& dep, size_t dim=0);
    void        default_indep_vect_vectorfield(const std::string& indep, const std::string& dep, const std::string& depy, size_t dim=0);
    void        default_indep_box_plotvect(const std::string& indep, const std::string& dep);
    void        clear_prev_var(std::string varname);

    void        populate_plot(plot_descriptor pl);
    void        populate_boxplot( plot_descriptor pl);
    void        populate_area( plot_descriptor pl);
    void        populate_barv( plot_descriptor pl);
    void        populate_scatter( plot_descriptor pl);
    void        populate_dens( plot_descriptor pl);
    void        populate_contour( plot_descriptor pl);
    void        populate_vect2d( plot_descriptor pl);
    void        populate_vertarrows( plot_descriptor pl);

    void        redraw(plot_descriptor& pl);

    JKQTPDatastore* get_ds();
    class           mdiOctaveInterface* _owner_mdiOctave;
    class           MainWindow*         _owner_mainWnd;

    bool            has_var(const QString& str_name);
};

#endif // WNDPLOT2D_H
