/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#ifndef RADARPROJECT_H
#define RADARPROJECT_H

#include <QVariant>
#include <memory>
#include <QFile>
#include <QtXml>
#include <set>
#include <string>
#define  XML_ARIAPROJECT_MAJOR  0
#define  XML_ARIAPROJECT_MINOR  1
#define  XML_ARIAPROJECT_SUBVER 0

enum dataType{DT_ROOT=0, DT_FOLDER=1, DT_RADARDEVICE=2, DT_RADARMODULE=3, DT_SCRIPT=4, DT_ANTENNA=5, DT_SCHEDULER=6, DT_PROTOCOL=7, DT_OTHER=8};

class radarModule;
class octaveScript;
class radarInstance;
class opScheduler;
class antenna;



class projectItem
{
private:
    dataType                       _item_type;
    QString                        _name;
    projectItem*                   itemParent;
    class radarProject*            _temp_root;  // This is intended to attach temporarily an item to a project
protected:
    QList<projectItem*>             qlChild;
    QString                         _filename;       // Relative path + filename (if folder, it is only relative path)
public:
    static projectItem*             create_new_item(projectItem* parent = nullptr, projectItem* leaf=nullptr);

    projectItem(projectItem* parent = nullptr);
    projectItem(QString name, dataType type, projectItem* parent = nullptr);
    ~projectItem();

    QString                             get_name() const {return _name;}
    void                                set_name(QString name) {_name = name;}

    const projectItem&                  operator = (const projectItem& item);

    projectItem*                        child_item(int n);
    int                                 child_count();
    dataType                            get_type();
    void                                add_child(projectItem *data);
    void                                remove_child(projectItem* item_to_remove);
    QString                             get_item_descriptor();
    projectItem*                        get_parent();
    class radarProject*                 get_root();
    projectItem*                        get_folder(QString folder_name);
    projectItem*                        get_parent(projectItem* item);
    projectItem*                        get_child(QString name, int type); // Use -1 to search for "any"
    void                                remove_all_children();
    QVector<projectItem*>               get_all_children(int type);// Use -1 to search for "any"

    QString                             get_full_path();
    QString                             get_full_filepath();
    QString                             get_filename() {return _filename;};
     void                               save_xml(QDomDocument& doc, QDomElement& root);
    QString                             get_item_relative_path_from(QString folder);
    //void                                   update_filename_relative_to_base(QString basedir);
    void                                move_to_new_basedir(QString new_basedir, QString relative_dir = "", bool keep_original_destination = true, bool copy = false);
    void                                set_temporary_project(radarProject* project);
    void                                remove_temporary_project();
    bool                                has_child(projectItem* item);
};


class folder : public projectItem
{
private:

public:
    folder(QString foldername, QString basedir="");
    ~folder() {};
    QString get_relative_path() {return _filename;}
    void       set_relative_path(const QString& path) {_filename = path;}
};


class radarProject : public QObject, public projectItem
{
    Q_OBJECT
private:
    QFile               _radar_project_file;
    QString             _last_error;
    bool                _b_loading;
    class octavews*     _workspace;

public:
    radarProject(QString name, class octavews* ws = nullptr, bool newProject = true);
    ~radarProject();
    void                set_workspace(class octavews* ws = nullptr);
    class octavews*     get_workspace() {return _workspace;}
//-----------------------------------
// Leafs
    projectItem*    get_root() {return this;}
    QString         get_name() {return projectItem::get_name();}

    void   add_module_scripts(radarModule* module);
    void   add_module_antennas(radarModule* module);
    projectItem*     load_item(QDomElement& xml_node, projectItem* parent = nullptr, projectItem* leaf=nullptr,  dataType datatype=DT_OTHER);
    projectItem*     load_radar_item(QDomElement& xml_node, projectItem* parent = nullptr, projectItem* leaf=nullptr);

    void    add_radar_module(QString &filename, projectItem* pitem=nullptr);
    void    update_radar_module(radarModule* module, projectItem* pitem=nullptr);
    void    update_radar_module(QString &filename, projectItem* pitem=nullptr);
    void    add_radar_module(radarModule* radar_module_ptr, projectItem* pitem=nullptr);
    bool    remove_radar_module(std::shared_ptr<radarModule> radar_module_to_remove, projectItem* pitem=nullptr);
    bool    remove_radar_module(radarModule* radar_module_to_remove, projectItem* pitem=nullptr);
    bool    remove_radar_module(QString &radar_module_name, projectItem* pitem=nullptr);

    octaveScript* add_script(QString &filename, projectItem* pitem=nullptr);
    void    add_script(octaveScript* radar_module_ptr, projectItem* pitem=nullptr);
    void    remove_script(std::shared_ptr<octaveScript> radar_module_to_remove, projectItem* pitem=nullptr);
    void    remove_script(QString &radar_module_name, projectItem* pitem=nullptr);

    bool    save_project_file();
    bool    load_project_file(QString filename,bool newProject = true);
    bool    load_project_file();
    void    clear();
    void    add_radar_instance(const QByteArray& uid, radarModule* module);
    void    add_radar_instance(radarInstance* radar_ptr);
    void    add_radar_instance(QString filename);
    void    remove_radar_instance(radarInstance* radar_ptr, projectItem* pitem=nullptr);
    void    remove_radar_instance(QString radarName, projectItem* pitem=nullptr);

    antenna* add_antenna(QString filename);
    antenna* add_antenna(antenna* instance);
    void     remove_antenna(QString filename);


    void             add_scheduler(opScheduler* scheduler);
    opScheduler*     add_scheduler(QString filename);
    void     remove_scheduler(opScheduler* scheduler, projectItem* pitem=nullptr);
    void     update_radar_module_devices(projectItem* moduleitem);

    QString get_last_error();

    QVector<radarModule*>                       get_available_modules();
    QVector<octaveScript*>                      get_available_scripts();
    QVector<radarInstance*>                     get_available_radars();
    QVector<class antenna*>                     get_available_antennas();
    QVector<opScheduler*>                       get_available_scheduler();
public slots:
    void            immediate_variable_updated(const std::string& varname);
    void            immediate_inquiry(const std::string& varname);
    void            immediate_command(const std::string& varname);

    void            variables_updated(const std::set<std::string>& varlist);
    void            errorWhileRunning();

signals:
    void            item_updated(projectItem* ptr);
    void            project_updated(projectItem* ptr=nullptr);
    void            add_item(projectItem* parent, projectItem* item);
    void            remove_item(projectItem* parent, projectItem* item);



};


#endif // RADARPROJECT_H
