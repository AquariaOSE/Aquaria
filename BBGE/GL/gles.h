/* OpenGL ES-specific. --ryan. */
/* I should probably put an official GLES header in here instead. Oh well. */

#ifdef __cplusplus
extern "C" {
#endif

void glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);
void glClearDepthf(GLclampf depth);

#ifdef __cplusplus
}
#endif

