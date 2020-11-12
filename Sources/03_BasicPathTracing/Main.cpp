#include <iostream>
#include <windows.h>
#include <memory>
#include <vector>
#include <ctime>
#include <chrono>
#include <random>

#include <Math.hpp>
#include <Ray.hpp>
#include <ImageBuffer.hpp>

#include "Types.h"
#include "Sphere.hpp"

#define MAX_RECURSION_DEPTH 50
#define MULTISAMPLING_LEVEL 16

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

/** U T I L S  **/

/**
 * \brief Случайное float значение
 * \param min Минимальная граница
 * \param max Максимальная границе
 * \return Случайное значение
 */
inline float RndFloat(float min = 0.0f, float max = 1.0f){
    static std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    static std::mt19937 generator(ms.count());
    static std::uniform_real_distribution<float> distribution(min, max);

    return distribution(generator);
}

/**
 * \brief Случайный вектор в заданых пределах
 * \param min Минимальное значение всех координат
 * \param max Максимальное значение всех координат
 * \return Вектор
 */
inline math::Vec3<float> RndVec3(float min = -1.0f, float max = 1.0f){
    static std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    static std::mt19937 generator(ms.count());
    static std::uniform_real_distribution<float> distribution(min, max);

    return {
        distribution(generator),
        distribution(generator),
        distribution(generator)
    };
}

/**
 * \brief Случайная точка в пределах единичной сферы
 * \return Вектор
 */
inline math::Vec3<float> RndUnitSpherePoint(){
    while (true){
        auto p = RndVec3();
        if(math::LengthSquared(p) >= 1) continue;
        return p;
    }
}

/**
 * \brief Случайный вектор в пределах полусферы направленной в направлении a
 * \param a Направление (ориентация) полусферы
 * \return Вектор
 */
inline math::Vec3<float> RndHemisphereVec(const math::Vec3<float>& a)
{
    // Генератор случайных чисел
    static std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    static std::mt19937 generator(ms.count());

    // Коэффициенты распределения для двух уголов
    std::uniform_real_distribution<float> fiDist(0.0f,1.0f);
    std::uniform_real_distribution<float> thetaDist(-1.0f, 1.0f);

    // Вектор перпендикулярный вектору направления
    auto b = math::Normalize(math::Cross(a,a + math::Vec3<float>(0.01f,0.01f,0.01f)));

    // Второй перпендикулярный вектор
    auto c = math::Normalize(math::Cross(a, b));

    // Случайные углы (fi - для вектора вращяющегося вокруг направления a, fi - угол между итоговым вектором и направлением a)
    float fi = (fiDist(generator) * 360.0f) / 57.2958f; // [0 - 360]
    float theta = (thetaDist(generator) * 90.0f) / 57.2958f; // [-90 - 90]

    // Вектор описывающий круг вокруг направления a
    math::Vec3<float> d = (b * std::cos(fi)) + (c * std::sin(fi));
    // Вектор отклоненный от a на случайный угол
    math::Vec3<float> e = (a * std::cos(theta)) + (d * std::sin(theta));

    return e;
}

/** R A Y T R A C I N G  **/

/**
 * \brief Метод рендеринга сцены
 * \param imageBuffer Целевой буфер изображения
 * \param scene Сцена
 * \param fov Угол обзора
 * \param samplerPerPixel Кол-во семплов (лучей) на один пиксель кадрового буфера
 *
 * \details В данном методе происходит генерация лучей для каждого пикселя кадрового буфера и последующая
 * трассировка лучами сцены, а также запись полученных значений в пиксели кадрового буфера
 */
void Render(
        ImageBuffer<RGBQUAD> *imageBuffer,
        const Scene& scene,
        const float& fov,
        unsigned samplerPerPixel = 1);

/**
 * \brief Метод трассировки сцены лучом
 * \param ray Луч
 * \param hittableElement Трассируемый элемент
 * \param outColor Результирующий цвет для точки пересечения
 * \param recursionDepth Глубина рекурсии
 * \return Было ли пересечение с каким-либо объектом сцены
 */
bool TraceTay(
        const math::Ray& ray,
        const HittableElement& hittableElement,
        math::Vec3<float>* outColor,
        unsigned recursionDepth = 0);

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
                640, 480,
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

        /** RAYTRACING **/

        // Создать буффер кадра
        auto frameBuffer = ImageBuffer<RGBQUAD>(clientRect.right, clientRect.bottom, {0, 0, 0, 0});
        std::cout << "INFO: Frame-buffer initialized  (resolution : " << frameBuffer.getWidth() << "x" << frameBuffer.getHeight() << ", size : " << frameBuffer.getSize() << " bytes)" << std::endl;

        // Сцена
        Scene scene{};
        scene.addElement(std::make_shared<Sphere>(math::Vec3<float>(0.0f,0.0f,-1.0f),0.5f));
        scene.addElement(std::make_shared<Sphere>(math::Vec3<float>(0,-100.5f,-1.0),100.0f));

        // Трассировка сцены лучами, запись результата в буфер изображения
        auto renderBeginTime = std::chrono::system_clock::now();
        Render(&frameBuffer, scene, 90.0f, MULTISAMPLING_LEVEL);
        std::cout << "INFO: Scene rendered in " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - renderBeginTime).count() << " ms." << std::endl;


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
 * \param scene Сцена
 * \param fov Угол обзора
 * \param samplerPerPixel Кол-во семплов (лучей) на один пиксель кадрового буфера
 *
 * \details В данном методе происходит генерация лучей для каждого пикселя кадрового буфера и последующая
 * трассировка лучами сцены, а также запись полученных значений в пиксели кадрового буфера
 */
void Render(ImageBuffer<RGBQUAD> *imageBuffer, const Scene& scene, const float &fov, unsigned samplerPerPixel)
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
            // Итоговый цвет пикселя
            math::Vec3<float> pixelColor = {0.0f, 0.0f, 0.0f};
            
            // Проход по семплам пикселя
            for(unsigned s = 0; s < samplerPerPixel; s++)
            {
                // Отклонение луча в пределах пикселя
                // В случае мультисемплинга генерируется случайный сдвинг, в противном случае сдвиг устанавливается в центр пикселя
                math::Vec2<float> pixelBias = (samplerPerPixel > 1 ? math::Vec2<float>(RndFloat(), RndFloat()) : math::Vec2<float>(0.5f, 0.5f));

                // Вычислить отклонение луча для текущего пикселя по углу обзора и текущим координатам пикселя
                float x = (2.0f * (static_cast<float>(i) + pixelBias.x) / w - 1.0f) * tanf(fovRadians / 2.0f) * w / h;
                float y = -(2.0f * (static_cast<float>(j) + pixelBias.y) / h - 1.0f) * tanf(fovRadians / 2.0f);

                // Создать луч
                math::Ray ray({0.0f,0.0f,0.0f},{x,y,-1.0f});

                // Трассировка сцены и получение цвета
                math::Vec3<float> sampleColor = {0.0f,0.0f,0.0f};
                TraceTay(ray, scene, &sampleColor);

                // Прибавить к итоговому цвету цвет семпла
                pixelColor = pixelColor + sampleColor;
            }

            // Поделить цвет на кол-во семплов
            pixelColor = pixelColor / static_cast<float>(samplerPerPixel);

            // Гамма коррекция (для гаммы в 2.0)
            pixelColor = {
                    std::sqrt(pixelColor.r),
                    std::sqrt(pixelColor.g),
                    std::sqrt(pixelColor.b),
            };

            // Установка цвета
            imageBuffer->setPoint(i,j,{
                    static_cast<BYTE>(math::Clamp(pixelColor.b, 0.0f, 1.0f) * 255.0f),
                    static_cast<BYTE>(math::Clamp(pixelColor.g, 0.0f, 1.0f) * 255.0f),
                    static_cast<BYTE>(math::Clamp(pixelColor.r, 0.0f, 1.0f) * 255.0f),
                    255
            });
        }
    }
}

/**
 * \brief Метод трассировки сцены лучом
 * \param ray Луч
 * \param hittableElement Трассируемый элемент
 * \param outColor Результирующий цвет для точки пересечения
 * \param unsigned recursionDepth
 * \return Было ли пересечение с каким-либо объектом сцены
 */
bool TraceTay(const math::Ray &ray, const HittableElement& hittableElement, math::Vec3<float> *outColor, unsigned recursionDepth)
{
    // Информация о пересечении с объектом
    HitInfo hitInfo{};

    // Если пересечение было
    if(recursionDepth < MAX_RECURSION_DEPTH && hittableElement.intersectsRay(ray,0.001f,1000.0f,&hitInfo))
    {
        /*
        // Цель для переотраженного уча (случайная точка внутри единичной сферы рядом с точкой пересечения)
        math::Vec3<float> target = hitInfo.point + hitInfo.normal + RndUnitSpherePoint();
        // Вектор направления переотраженного луча
        math::Vec3<float> rayDir = target - hitInfo.point;
        */

        // Вектор направления переотраженного луча (случайное направление в пределах полусферы)
        math::Vec3<float> rayDir = RndHemisphereVec(hitInfo.normal);

        // Случайный переотраженный луч
        math::Ray newRay(hitInfo.point, rayDir);

        // Трассировать сцену переотраженным лучом и получить цвет
        math::Vec3<float> color{};
        TraceTay(newRay,hittableElement,&color,recursionDepth + 1);

        // Цвет для вторичного луча делится на 2
        *outColor = color * 0.5f;
        return true;
    }

    // Коэффициент интерполяции цветов (чем выше направлен вектор, тем он больше уходит в синий)
    auto h = 0.5*(ray.getDirection().y + 1.0);
    // Итоговый цвет (интерполяция между белым и синим)
    *outColor = math::Mix(math::Vec3<float>(1.0f,1.0f,1.0f),math::Vec3<float>(0.5f, 0.7f, 1.0f),h);
    // Пересечение не засчитано
    return false;
}