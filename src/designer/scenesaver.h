#ifndef SCENESAVER_H
#define SCENESAVER_H

class Scene;
class QString;

void save_scene(Scene *scene, const QString &file_name);
void open_scene(Scene *scene, const QString &file_name);
void export_scene_to_particle_json(Scene *scene,const QString &file_name);

#endif // SCENESAVER_H
