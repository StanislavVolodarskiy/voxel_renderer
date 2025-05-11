// g++ -std=c++23 -pedantic -Wall -Wextra -Werror -Wwrite-strings -Wconversion -ovoxel-cpp voxel.cpp

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>


#include <gtk/gtk.h>

struct Image {
    using Pixel = unsigned char[3];

    Image(int height, int width)
    : height(height), width(width), data(height * width) {}

    Pixel *operator[](int i) { return data.data() + width * i; }

    void save(const char *filename) const {
        std::ofstream ofs(filename, std::ios::binary);
        ofs << "P6\n" << width << ' ' << height << "\n255\n";
        ofs.write(
            reinterpret_cast<const char *>(data.data()),
            height * width * sizeof(Pixel)
        );
    }

    int height;
    int width;
private:
    std::vector<Pixel> data;
};

void help() {
    std::cout << "Commands:\n\n";
    std::cout << "help            shows help\n";
    std::cout << "test-ppm        renders sample of ppm image\n";
    std::cout << "test-gtk        opens GTK window\n";
}

void test_ppm() {
    Image image(480, 640);
    for (int i = 0; i < image.height; ++i) {
        for (int j = 0; j < image.width; ++j) {
            image[i][j][0] = 255;
            image[i][j][1] = 255;
            image[i][j][2] = 255;
        }
    }
    for (int i = 100; i < image.height - 50; ++i) {
        for (int j = 200; j < image.width - 100; ++j) {
            image[i][j][0] = 255;
            image[i][j][1] = 0;
            image[i][j][2] = 0;
        }
    }
    image.save("voxel-cpp.ppm");
}

void test_gtk() {
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        help();
    }
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "help") == 0) {
            help();
        } else if (std::strcmp(argv[i], "test-ppm") == 0) {
            test_ppm();
        } else if (std::strcmp(argv[i], "test-gtk") == 0) {
            test_gtk();
        } else {
            std::cout << "What is " << argv[i] << "?\n";
            help();
            std::exit(1);
        }
    }
}
