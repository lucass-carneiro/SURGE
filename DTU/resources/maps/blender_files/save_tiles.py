import bpy
import os

def hide_all_meshes():
    for obj in bpy.data.objects:
        if obj.type == "MESH":
            obj.hide_render = True


def show_all_meshes():
    for obj in bpy.data.objects:
        if obj.type == "MESH":
            obj.hide_render = False
    
def render_tiles():
    for col in bpy.data.collections:
        col_name = col.name
        
        if not os.path.exists(col_name):
            os.mkdir(col_name)
    
        for obj in bpy.data.collections[col_name].objects:
            obj_name = obj.name
            tile_img_name = "{}_{}.png".format(col_name, obj_name)
            tile_img_path = os.path.join(col_name, tile_img_name)
            
            obj.hide_render = False
            
            bpy.data.scenes["Scene"].render.filepath = tile_img_path
            bpy.ops.render.render(write_still=True)
            
            obj.hide_render = True

hide_all_meshes()
render_tiles()
show_all_meshes()