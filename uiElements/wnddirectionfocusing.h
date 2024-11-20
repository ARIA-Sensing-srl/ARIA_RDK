/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#ifndef WNDDIRECTIONFOCUSING_H
#define WNDDIRECTIONFOCUSING_H

#include <QDialog>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkNamedColors.h>
#include <vtkCubeAxesActor2D.h>
#include <vtkDoubleArray.h>
#include <vtkPolyDataNormals.h>
#include <vtkOutlineFilter.h>
#include <vtkScalarBarActor.h>
#include <vtkLookupTable.h>
#include <vtkPointSource.h>
#include <vtkUnstructuredGrid.h>
#include <vtkDataSetMapper.h>
#include <vtkGlyph3DMapper.h>

#include "radarmodule.h"
namespace Ui {
class wndDirectionFocusing;
}

class wndDirectionFocusing : public QDialog
{
    Q_OBJECT

public:
    explicit wndDirectionFocusing(QWidget *parent = nullptr, radarModule* radar_module_ptr = nullptr);
    ~wndDirectionFocusing();

    void fill_antenna_data();


private:
    Ui::wndDirectionFocusing *ui;

    vtkSmartPointer<vtkPoints>              _points;
    vtkSmartPointer<vtkPolyData>            _polydata;
    vtkSmartPointer<vtkCellArray>           _cells;
    vtkSmartPointer<vtkPolyDataMapper>      _polydatamapper;
    vtkSmartPointer<vtkNamedColors>         _colors;
    vtkSmartPointer<vtkActor>               _actor;
    vtkSmartPointer<vtkRenderer>            _renderer;
    vtkSmartPointer<vtkCubeAxesActor2D>     _axes;
    vtkSmartPointer<vtkDoubleArray>         _gain;
    vtkSmartPointer<vtkPolyDataNormals>     _normals;
    vtkSmartPointer<vtkOutlineFilter>       _outline_filter;
    vtkSmartPointer<vtkScalarBarActor>      _color_bar;
    vtkSmartPointer<vtkLookupTable>         _lookup_table;
    vtkSmartPointer<vtkTextMapper>          _text_mapper;
    vtkSmartPointer<vtkActor2D>             _text_actor;
    vtkSmartPointer<vtkUnstructuredGrid>    _line_grid;
    vtkSmartPointer<vtkDataSetMapper>       _line_mapper;
    vtkSmartPointer<vtkActor>               _line_actor;
    vtkSmartPointer<vtkGlyph3DMapper>       _points_mapper;
    vtkSmartPointer<vtkActor>               _glyphs_actor;

    radarModule             *_radar_module;
    void                     update_graph();
    void                     init_sliders();
public slots:
    void                     slide_azimuth(int tick=0);
    void                     slide_zenith(int tick=0);
    void                     slide_freq(int tick=0);



};

#endif // WNDDIRECTIONFOCUSING_H
