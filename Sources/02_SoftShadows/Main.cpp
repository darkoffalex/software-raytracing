#include <iostream>
#include <windows.h>
#include <memory>
#include <limits>
#include <random>
#include <chrono>

#include <ImageBuffer.hpp>

#include "Types.h"
#include "Sphere.hpp"
#include "Plane.hpp"

#define MAX_RECURSION_DEPTH 4
#define MAX_SHADOW_SAMPLES 16

/**
 * Коды ошибок
 */
enum ErrorCode
{
    eNoErrors,
    eClassRegistrationError,
    eWindowCreationError,
};

/// Дескриптор исполняемого модуля программы
HINSTANCE g_hInstance = nullptr;
/// Дескриптор осноного окна отрисовки
HWND g_hwnd = nullptr;
/// Дескриптор контекста отрисовки
HDC g_hdc = nullptr;
/// Наименование класса
const char* g_strClassName = "MainWindowClass";
/// Заголовок окна
const char* g_strWindowCaption = "Basic software raytracing";
/// Код последней ошибки
ErrorCode g_lastError = ErrorCode::eNoErrors;
/// Генератор случайных чисел
std::default_random_engine* g_rndEngine = nullptr;

/** W I N A P I  S T U F F **/

/**
 * \brief Обработчик оконных сообщений
 * \param hWnd Дескриптор окна
 * \param message Сообщение
 * \param wParam Параметр сообщения
 * \param lParam Параметр сообщения
 * \return Код выполнения
 */
LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

/**
 * \brief Копирование информации о пикселях изображения в буфер "поверхности" окна
 * \param pixels Массив пикселей
 * \param width Ширина области
 * \param height Высота области
 * \param hWnd Дескриптор окна
 */
void PresentFrame(void *pixels, int width, int height, HWND hWnd);

/** R A Y T R A C I N G  **/

/**
 * \brief Атрибуты точки ближайшего пересечения
 */
struct NearestHit
{
    /// Расстояние до точки пересечения
    float distance = 0;
    /// Нормаль поверхностив в точке пересечения
    math::Vec3<float> normal = {0.0f,0.0f,0.0f};
    /// Индекс объекта
    unsigned instanceIndex = 0;
};

/**
 * \brief Метод рендеринга сцены
 * \param imageBuffer Целевой буфер изображения
 * \param fov Угол обзора
 * \param sceneElements Элементы сцены
 * \param lightSources Источники освещения
 *
 * \details В данном методе происходит генерация лучей для каждого пикселя кадрового буфера и последующая
 * трассировка лучами сцены, а также запись полученных значений в пиксели кадрового буфера
 */
void Render(
        ImageBuffer<RGBQUAD> *imageBuffer,
        const float& fov,
        const std::vector<std::shared_ptr<SceneElement>> &sceneElements,
        const std::vector<LightSource>& lightSources);

/**
 * \brief Метод трассировки сцены лучом
 * \param ray Луч
 * \param sceneElements Элементы сцены
 * \param lightSources Источники освещения
 * \param minDistance Минимальное расстояние
 * \param maxDistance Максимальное расстояние
 * \param outColor Результирующий цвет для точки пересечения
 * \param recursionDepth Глубина рекурсии (значение должно увеличиваться на единицу при каждом рекурсивном вызове)
 * \return Было ли пересечение с каким-либо объектом сцены
 */
bool TraceTay(
        const math::Ray& ray,
        const std::vector<std::shared_ptr<SceneElement>> &sceneElements,
        const std::vector<LightSource>& lightSources,
        float minDistance,
        float maxDistance,
        math::Vec3<float>* outColor,
        uint32_t recursionDepth = 0);

/**
 * \brief Получить случайный вектор в направлении сферы источника сфера (в пределах конуса)
 * \param shadedPoint Положение затеняемой точки
 * \param lightSource Сферический источник света
 * \return Вектор направления
 */
math::Vec3<float> RandomVectorToLightSphere(
        const math::Vec3<float>& shadedPoint,
        const LightSource& lightSource);

/** M A I N **/

/**
 * \brief Точка входа
 * \param argc Кол-во аргументов
 * \param argv Аргмуенты
 * \return Код исполнения
 */
int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    try {
        // Получение дескриптора исполняемого модуля программы
        g_hInstance = GetModuleHandle(nullptr);

        // Информация о классе
        WNDCLASSEX classInfo;
        classInfo.cbSize = sizeof(WNDCLASSEX);
        classInfo.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        classInfo.cbClsExtra = 0;
        classInfo.cbWndExtra = 0;
        classInfo.hInstance = g_hInstance;
        classInfo.hIcon = LoadIcon(g_hInstance, IDI_APPLICATION);
        classInfo.hIconSm = LoadIcon(g_hInstance, IDI_APPLICATION);
        classInfo.hCursor = LoadCursor(nullptr, IDC_ARROW);
        classInfo.hbrBackground = CreateSolidBrush(RGB(240, 240, 240));
        classInfo.lpszMenuName = nullptr;
        classInfo.lpszClassName = g_strClassName;
        classInfo.lpfnWndProc = WindowProcedure;

        // Пытаемся зарегистрировать оконный класс
        if (!RegisterClassEx(&classInfo)) {
            g_lastError = ErrorCode::eClassRegistrationError;
            throw std::runtime_error("ERROR: Can't register window class.");
        }

        // Создание окна
        g_hwnd = CreateWindow(
                g_strClassName,
                g_strWindowCaption,
                WS_OVERLAPPEDWINDOW,
                0, 0,
                800, 600,
                nullptr,
                nullptr,
                g_hInstance,
                nullptr);

        // Если не удалось создать окно
        if (!g_hwnd) {
            g_lastError = ErrorCode::eWindowCreationError;
            throw std::runtime_error("ERROR: Can't create main application window.");
        }

        // Показать окно
        ShowWindow(g_hwnd, SW_SHOWNORMAL);

        // Получение контекста отрисовки
        g_hdc = GetDC(g_hwnd);

        // Размеры клиентской области окна
        RECT clientRect;
        GetClientRect(g_hwnd, &clientRect);

        // Генератор случайных чисел
        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
        g_rndEngine = new std::default_random_engine(static_cast<unsigned>(ms.count()));

        /** RAYTRACING **/

        // Создать буффер кадра
        auto frameBuffer = ImageBuffer<RGBQUAD>(clientRect.right, clientRect.bottom, {0, 0, 0, 0});
        std::cout << "INFO: Frame-buffer initialized  (resolution : " << frameBuffer.getWidth() << "x" << frameBuffer.getHeight() << ", size : " << frameBuffer.getSize() << " bytes)" << std::endl;

        // Материалы
        Material redMatte = {math::Vec3<float>(1.0f, 0.0f, 0.0f),0.0f};
        Material greenMatte = {math::Vec3<float>(0.0f, 1.0f, 0.0f),0.0f};
        Material whiteMatte = {math::Vec3<float>(1.0f, 1.0f, 1.0f),0.0f};
        Material redRubber = {math::Vec3<float>(0.6f, 0.2f, 0.2f),0.1f,16.0f};

        // Сцена
        std::vector<std::shared_ptr<SceneElement>> scene{};
        scene.push_back(std::make_shared<Plane>(whiteMatte,math::Vec3<float>(0.0f, -10.0f, 0.0f),math::Vec3<float>(0.0f, 1.0f, 0.0f)));
        scene.push_back(std::make_shared<Plane>(whiteMatte,math::Vec3<float>(0.0f, 10.0f, 0.0f),math::Vec3<float>(0.0f, -1.0f, 0.0f)));
        scene.push_back(std::make_shared<Plane>(whiteMatte,math::Vec3<float>(0.0f, 0.0f, -20.0f),math::Vec3<float>(0.0f, 0.0f, 1.0f)));
        scene.push_back(std::make_shared<Plane>(whiteMatte,math::Vec3<float>(0.0f, 0.0f, 0.001f),math::Vec3<float>(0.0f, 0.0f, -1.0f)));
        scene.push_back(std::make_shared<Plane>(greenMatte,math::Vec3<float>(10.0f, 0.0f, 00.0f),math::Vec3<float>(-1.0f, 0.0f, 0.0f)));
        scene.push_back(std::make_shared<Plane>(redMatte,math::Vec3<float>(-10.0f, 0.0f, 00.0f),math::Vec3<float>(1.0f, 0.0f, 0.0f)));
        scene.push_back(std::make_shared<Sphere>(redRubber,math::Vec3<float>(0.0f,-6.5f,-15.0f),3.0f));

        // Источники света
        std::vector<LightSource> lightSources{};
        lightSources.push_back({math::Vec3<float>(0.0f,6.5f,-10.0f),math::Vec3<float>(0.9f,0.9f,0.9f),3.0f});

        // Трассировка сцены лучами, запись результата в буфер изображения
        Render(&frameBuffer,90.0f, scene, lightSources);

        // Показ кадра
        PresentFrame(frameBuffer.getData(), static_cast<int>(frameBuffer.getWidth()), static_cast<int>(frameBuffer.getHeight()), g_hwnd);

        /** MAIN LOOP **/

        // Оконное сообщение
        MSG msg = {};

        // Запуск цикла
        while (true)
        {
            // Обработка оконных сообщений
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                DispatchMessage(&msg);

                if (msg.message == WM_QUIT) {
                    break;
                }
            }
        }
    }
    catch(std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
    }

    // Уничтожение генератора случайных чисел
    delete g_rndEngine;
    // Уничтожение окна
    DestroyWindow(g_hwnd);
    // Вырегистрировать класс окна
    UnregisterClass(g_strClassName, g_hInstance);

    // Код выполнения/ошибки
    return static_cast<int>(g_lastError);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** W I N A P I  S T U F F **/

/**
 * \brief Обработчик оконных сообщений
 * \param hWnd Дескриптор окна
 * \param message Сообщение
 * \param wParam Параметр сообщения
 * \param lParam Параметр сообщения
 * \return Код выполнения
 */
LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if(message == WM_DESTROY){
        PostQuitMessage(0);
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

/**
 * \brief Копирование информации о пикселях изображения в буфер "поверхности" окна
 * \param pixels Массив пикселей
 * \param width Ширина области
 * \param height Высота области
 * \param hWnd Дескриптор окна
 */
void PresentFrame(void *pixels, int width, int height, HWND hWnd)
{
    // Получить хендл на временный bit-map (4 байта на пиксель)
    HBITMAP hBitMap = CreateBitmap(
            width,
            height,
            1,
            8 * 4,
            pixels);

    // Получить device context окна
    HDC hdc = GetDC(hWnd);

    // Временный DC для переноса bit-map'а
    HDC srcHdc = CreateCompatibleDC(hdc);

    // Связать bit-map с временным DC
    SelectObject(srcHdc, hBitMap);

    // Копировать содержимое временного DC в DC окна
    BitBlt(
            hdc,                                  // HDC назначения
            0,                                 // Начало вставки по оси X
            0,                                 // Начало вставки по оси Y
            static_cast<int>(width),              // Ширина
            static_cast<int>(height),             // Высота
            srcHdc,                               // Исходный HDC (из которого будут копироваться данные)
            0,                                // Начало считывания по оси X
            0,                                // Начало считывания по оси Y
            SRCCOPY                               // Копировать
    );

    // Уничтожить bit-map
    DeleteObject(hBitMap);
    // Уничтожить временный DC
    DeleteDC(srcHdc);
    // Уничтожить DC
    ReleaseDC(hWnd,hdc);
}

/** R A Y T R A C I N G  M E T H O D S **/

/**
 * \brief Метод рендеринга сцены
 * \param imageBuffer Целевой буфер изображения
 * \param fov Угол обзора
 * \param sceneElements Элементы сцены
 * \param lightSources Источники освещения
 *
 * \details В данном методе происходит генерация лучей для каждого пикселя кадрового буфера и последующая
 * трассировка лучами сцены, а также запись полученных значений в пиксели кадрового буфера
 */
void Render(ImageBuffer<RGBQUAD> *imageBuffer, const float &fov,
            const std::vector<std::shared_ptr<SceneElement>> &sceneElements,
            const std::vector<LightSource> &lightSources)
{
    // Размеры кадрового буфера
    auto w = static_cast<float>(imageBuffer->getWidth());
    auto h = static_cast<float>(imageBuffer->getHeight());

    // Угол обзора в радианах
    auto fovRadians = static_cast<float>(fov / (180.0f / M_PI));

    // Проход по пикселям кадрового буфера
    for(uint32_t j = 0; j < imageBuffer->getHeight(); j++) {
        for (uint32_t i = 0; i < imageBuffer->getWidth(); i++)
        {
            // Вычислить отклонение луча для текущего пикселя по углу обзора и текущим координатам пикселя
            float x = (2.0f*(static_cast<float>(i) + 0.5f)/w  - 1.0f) * tanf(fovRadians/2.0f) * w/h;
            float y = -(2.0f*(static_cast<float>(j) + 0.5f)/h - 1.0f) * tanf(fovRadians/2.0f);

            // Создать луч
            math::Ray ray({0.0f,0.0f,0.0f},{x,y,-1.0f});

            // Трассировка сцены и получение цвета
            math::Vec3<float> resultColor = {0.0f,0.0f,0.0f};
            TraceTay(ray, sceneElements, lightSources,0.0f,1000.0f, &resultColor);

            // Установка цвета
            imageBuffer->setPoint(i,j,{
                    static_cast<BYTE>(math::Clamp(resultColor.b,0.0f,1.0f) * 255.0f),
                    static_cast<BYTE>(math::Clamp(resultColor.g,0.0f,1.0f) * 255.0f),
                    static_cast<BYTE>(math::Clamp(resultColor.r,0.0f,1.0f) * 255.0f),
                    255
            });
        }
    }
}

/**
 * \brief Метод трассировки сцены лучом
 * \param ray Луч
 * \param sceneElements Элементы сцены
 * \param lightSources Источники освещения
 * \param minDistance Минимальное расстояние
 * \param maxDistance Максимальное расстояние
 * \param outColor Результирующий цвет для точки пересечения
 * \param recursionDepth Глубина рекурсии (значение должно увеличиваться на единицу при каждом рекурсивном вызове)
 * \return Было ли пересечение с каким-либо объектом сцены
 */
bool TraceTay(const math::Ray &ray, const std::vector<std::shared_ptr<SceneElement>> &sceneElements,
              const std::vector<LightSource> &lightSources, float minDistance, float maxDistance,
              math::Vec3<float> *outColor, uint32_t recursionDepth)
{
    // Было ли пересечение
    bool hit = false;

    // Атрибуты точки ближайшего пересечения
    NearestHit nearestHit{};
    nearestHit.distance = std::numeric_limits<float>::max();
    nearestHit.normal = {0.0f,0.0f,0.0f};
    nearestHit.instanceIndex = 0;

    // Если глубина рекурсии не была превышена
    if(recursionDepth <= MAX_RECURSION_DEPTH)
    {
        // Пройтись по объектам сцены
        for(uint32_t i = 0; i < sceneElements.size(); i++)
        {
            // Расстояние до точки пересечения и нормаль в точке пересечения
            float t; math::Vec3<float> normal;
            if(sceneElements[i]->intersectsRay(ray, minDistance, maxDistance, &t, &normal) && t < nearestHit.distance)
            {
                // Пересечение засчитано
                hit = true;

                // Перезаписать атрибуты ближайшего пересечения
                nearestHit.distance = t;
                nearestHit.normal = normal;
                nearestHit.instanceIndex = i;
            }
        }
    }

    // Если было пересечение и если необходимо вычислить цвет
    // Следующую часть кода можно считать аналогом шейдера ray-hit (closest-hit)
    if(hit && outColor != nullptr)
    {
        // Объект с которым было ближайшее пересечения
        const auto& sceneElement = sceneElements[nearestHit.instanceIndex];

        // Точка пересечения
        math::Vec3<float> intersectionPoint = ray.getOrigin() + (ray.getDirection() * nearestHit.distance);

        // Компоненты итогового цвета (основные - диффузный и бликовый, второстепенные - отражение и преломление)
        math::Vec3<float> primary = {0.0f,0.0f,0.0f};
        math::Vec3<float> secondary = {0.0f,0.0f,0.0f};

        // Если нужно считать основные компонентв освещенности
        if(sceneElement->getMaterial().primaryToSecondary > 0.0f)
        {
            // Дифузная и бликовая компоненты
            math::Vec3<float> diffuse = {0.0f,0.0f,0.0f};
            math::Vec3<float> specular = {0.0f,0.0f,0.0f};

            // Проход по всем источникам света
            for(const auto& lightSource : lightSources)
            {
                // Векторы от точки пересечения до источника (обычный и нормированный)
                math::Vec3<float> toLight = lightSource.position - intersectionPoint;
                math::Vec3<float> toLightDir = math::Normalize(toLight);

                // Интенсивности освещенности точки данным источником
                float intensity = 1.0f;
                // Доля интенсивности которая приходится на один теневой луч (сэмпл)
                float intensityPerSample = 1.0f / static_cast<float>(MAX_SHADOW_SAMPLES);

                // Запустить лучи в сторону источника света
                for(unsigned i = 0; i < MAX_SHADOW_SAMPLES; i++)
                {
                    // Случайное направление в сторону сферического источника (в пределах конуса образованного диском сферы)
                    math::Vec3<float> rndToLight = RandomVectorToLightSphere(intersectionPoint,lightSource);

                    // Если луч пересекся с преградой - убавить интенсивность на соответствующую часть
                    math::Ray shadowRay(intersectionPoint, rndToLight);
                    if(TraceTay(shadowRay,sceneElements,{},0.01f,math::Length(toLight) - lightSource.radius, nullptr, recursionDepth + 1)){
                        intensity -= intensityPerSample;
                    }
                }

                // Если луч запущенный в сторону источника пересекся с геометрией сцены - пропуск источника
                //math::Ray shadowRay(intersectionPoint, toLight);
                //if(TraceTay(shadowRay,sceneElements,{},0.01f,math::Length(toLight),nullptr,recursionDepth + 1)) continue;

                // Подсчет диффузной и бликовой компоненты (модель Фонга)
                diffuse = diffuse + (lightSource.color * std::max(0.0f, math::Dot(toLightDir,nearestHit.normal)) * intensity);
                specular = specular + (lightSource.color * std::pow(
                        std::max(0.0f,math::Dot(math::Reflect(-toLightDir,nearestHit.normal),-ray.getDirection())),
                        sceneElement->getMaterial().shininess) * intensity);
            }

            // Основная компонента (дифузный и бликовый свет)
            primary = (diffuse * sceneElement->getMaterial().albedo) + (specular * sceneElement->getMaterial().specularIntensity);
        }

        // Если нужно считать второстепенные компоненты (отражение и преломление)
        if(sceneElement->getMaterial().primaryToSecondary < 1.0f)
        {
            // Компоненты отражения и преломления
            math::Vec3<float> reflection = {0.0f,0.0f,0.0f};
            math::Vec3<float> refraction = {0.0f,0.0f,0.0f};

            // Если нужно считать отражение
            if(sceneElement->getMaterial().reflectToRefract > 0.0f)
            {
                math::Ray reflectedRay(intersectionPoint,math::Reflect(ray.getDirection(),nearestHit.normal));
                TraceTay(reflectedRay,sceneElements,lightSources,0.001f,maxDistance,&reflection,recursionDepth + 1);
            }

            // Если нужно считать преломление
            if(sceneElement->getMaterial().reflectToRefract < 1.0f)
            {
                // Коэффициент преломления
                float eta = sceneElement->getMaterial().refractionEta;

                // Если луч выходит из вещества - инвертировать нормаль и коэффициент преломления
                if(math::Dot(nearestHit.normal,-ray.getDirection()) < 0.0f){
                    nearestHit.normal = -nearestHit.normal;
                    eta = 1.0f / eta;
                }

                math::Ray refractedRay(intersectionPoint,math::Refract(ray.getDirection(),nearestHit.normal,eta));
                TraceTay(refractedRay,sceneElements,lightSources,0.001f,maxDistance,&refraction,recursionDepth + 1);
            }

            // Второстепенная компонента освещения
            secondary = math::Mix(reflection,refraction,std::max(1.0f - sceneElement->getMaterial().reflectToRefract, 0.0f));
        }

        // Итоговый цвет
        *outColor = math::Mix(primary,secondary,std::max(1.0f - sceneElement->getMaterial().primaryToSecondary, 0.0f));
    }

    // Если пересечения не было
    // Следующую часть кода можно считать аналогом шейдера ray-miss
    else if(outColor != nullptr)
    {
        // Отдать цвет для промаха (цвет неба)
        *outColor = {0.2f, 0.7f, 0.8f};
    }

    return hit;
}

/**
 * \brief Получить случайный вектор в направлении сферы источника сфера (в пределах конуса)
 * \param shadedPoint Положение затеняемой точки
 * \param lightSource Сферический источник света
 * \return Вектор направления
 */
math::Vec3<float> RandomVectorToLightSphere(
        const math::Vec3<float> &shadedPoint,
        const LightSource &lightSource)
{
    // Вектор от затенямой точки до центра источника сфера (нормированный)
    math::Vec3<float> toLight = math::Normalize(lightSource.position - shadedPoint);

    // Рандомизированные значения для вектора смещения и радиуса
    std::uniform_real_distribution<float> rBiasDist(0.0f,1.0f);
    std::uniform_real_distribution<float> vBiasDist(-1.0f, 1.0f);

    math::Vec3<float> vBias = {
            vBiasDist(*g_rndEngine),
            vBiasDist(*g_rndEngine),
            vBiasDist(*g_rndEngine)
    };

    float rBias = rBiasDist(*g_rndEngine);

    // Случайный перпендикулярный вектор к вектору от точки до источника
    math::Vec3<float> randomPl = math::Normalize(math::Cross(toLight, toLight + vBias));

    // Случайная точка на диске сферы источника света
    math::Vec3<float> randomPointOnLightDisk = lightSource.position + randomPl * lightSource.radius * rBias;

    // Вектор до случайной точки на диске источника света
    return math::Normalize(randomPointOnLightDisk - shadedPoint);
}
