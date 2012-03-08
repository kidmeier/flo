type:   shdr    self            write_Shader    read_Shader
type:   mesh    self            write_Mesh      read_Mesh
type:   skel    self            write_Skel      read_Skel
import: shdr    .vert           self    import_Shader
import: shdr    .frag           self    import_Shader
import: skel    .md5mesh        self    import_MD5
import: mesh    .obj            self    import_Obj
