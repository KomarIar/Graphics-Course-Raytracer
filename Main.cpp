#include "Tracer.h"
#include "stdio.h"

void main(int argc, char** argv) 
{
    CTracer tracer;
    CScene scene;
	Parameters param;
	tracer.m_param = &param;
	scene.m_param = &param;

    int xRes = 800;                 // Default resolution
    int yRes = 800;

    if (argc == 2) {                  // There is input file in parameters
        FILE* file = fopen(argv[1], "r");
		bool error = false;

        if (file) {

			int P1 = 0;
			int P2 = 0;

			float v1 = 0;
			float v2 = 0;   //переменные для считывания векторов
			float v3 = 0;

			//считываем разрешение
            if (fscanf(file, "%d %d", &P1, &P2) == 2) {
                xRes = P1;
                yRes = P2;
            }
			else {
				printf("Invalid config format! Using default parameters.\r\n");
				error = true;
			}

			//считываем данные камеры
			if (!error && fscanf (file, "%f %f %f", &v1, &v2, &v3) == 3) {
				param.Camera_Pos = glm::vec3(v1, v2, v3);
			}
			else {
				printf("Invalid config format! Setting remaining parameters to default. --->Camera Position\r\n");
				error = true;
			}
			if (!error && fscanf(file, "%f %f", &v1, &v2) == 2) {
				param.AngleY = v1;
				param.AngleX = v2;
			}
			else {
				printf("Invalid config format! Setting remaining parameters to default. --->Camera Angles\r\n");
				error = true;
			}
			//считываем данные фонарика
			if (!error && fscanf(file, "%f %f %f", &v1, &v2, &v3) == 3) {
				param.Light_Pos = glm::vec3(v1, v2, v3);
			}
			else {
				printf("Invalid config format! Setting remaining parameters to default. --->Light Position\r\n");
				error = true;
			}
			if (!error && fscanf(file, "%f %f %f", &v1, &v2, &v3) == 3) {
				param.Light_Dir = glm::vec3(v1, v2, v3);
			}
			else {
				printf("Invalid config format! Setting remaining parameters to default. --->Light Direction\r\n");
				error = true;
			}
			//считываем список включенных опций
			if (!error && fscanf(file, "%d", &P1) == 1) param.AA = P1;
			else {
				printf("Anti-aliasing Off");
				error = true;
			}
			if (!error && fscanf(file, "%d", &P1) == 1) param.OpticSystem = P1;
			else {
				printf("OpticSystem On");
				error = true;
			}
			if (!error && fscanf(file, "%d", &P1) == 1) param.Shadow = P1;
			else {
				printf("Shadows On");
				error = true;
			}
			if (!error && fscanf(file, "%d", &P1) == 1) param.OMP = P1;
			else {
				printf("Multiprocessing On");
				error = true;
			}
            fclose(file);
        }
        else
            printf("Invalid config path! Using default parameters.\r\n");
    }
    else
        printf("No config! Using default parameters.\r\n");

    scene.CreateScene();
    tracer.m_pScene = &scene;
    tracer.RenderImage(xRes, yRes);
    tracer.SaveImageToFile("../img/Result.png");
}