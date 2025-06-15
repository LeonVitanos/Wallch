# Wallch

At the age of 15, I took my first steps into programming with this project, together with my friend [Alexandros Solanos](https://github.com/hytromo).

<p align="center">
  <img src="https://i.imgur.com/j6vWRv1.jpg" alt="alt text">
</p>

Wallch is a powerful wallpaper changer. Besides changing pictures from local folders, it became popular in the open-source community for its unique features:

* **Live Earth Wallpaper:** A live wallpaper of the Earth that updates with the current satellite cloud and sunlight data.
* **Wikipedia's Picture of the Day:** Automatically sets your wallpaper to Wikipedia's featured image each day.
* **Wallpaper Clocks:** Wallpapers that change throughout the day to reflect the current time.

## Development Status: Wallch 5

Wallch 5 is in active development. I am modernizing the entire application by refactoring the original C++ codebase to meet modern best practices, with the primary goals of:
* Make all original features work reliably.
* Achieve full cross-platform compatibility (Windows, macOS, Linux).
* Prepare the project for a complete UI modernization using QML.
* Introduce new, modern features.

> 🧹 **Milestone (June 14, 2025):** The project now compiles error- and warning-free on Linux, Windows, and macOS for both Qt 5.15.2 and modern Qt 6 versions.

### Feature Status

| Feature                      | Status   | Notes                                       |
| :--------------------------- | :------: | :------------------------------------------ |
| Change from Local Images     |    ➖    | Partially works (not on all platforms yet). |
| Live Earth                   |    ❌    | Non-functional. A major goal.               |
| Wikipedia Picture of the Day |    ❌    | Non-functional. A major goal.               |
| Live Website                 |    ❌    | Non-functional.                             |
| Wallpaper Clocks             |    ❌    | Non-functional.                             |

### Platform Compatibility

Below is a snapshot of the current compatibility based on recent testing.

| OS / DE      | Tested On                    | Status | Change Image (Local) | Solid Color | Style/Sizing |
| :----------- | :--------------------------- | :----: | :------------------: | :---------: | :----------: |
| **macOS** | Monterey v12.7.6             |   ➖   |          ✅          |      ❌      |      ❌      |
| **Windows** | Windows 11                   |   ➖   |          ✅          |      ❌      |      ✅      |
| **Linux** |                              |        |                      |             |              |
| GNOME        | Ubuntu 22.04 & 25.04         |   ✅   |          ✅          |      ✅      |      ✅      |
| XFCE         | Linux Mint 21.1              |   ➖   |          ✅          |      ❌      |      ❌      |
| LXDE         | Fedora 37                    |   ❌   |          ❌          |      ❌      |      ❌      |
| LXDE         | Lubuntu 18.04                |   ❌   |          ❌          |      ❌      |      ❌      |
| KDE Plasma   | Kubuntu 22.04                |   ❌   |          ❌          |      ❌      |      ❌      |
| LXQt         | Lubuntu 22.04                |   ➖   |          ✅          |      ❌      |      ❌      |


### Reviews & Community

Wallch was featured in several online publications and has been translated into more than 7 languages with the help of the community.

* [OMG! Ubuntu!](https://www.omgubuntu.co.uk/2011/08/wallch-wallpaper-changer-adds-unity-features)
* [Web Upd8](http://www.webupd8.org/2014/06/wallch-use-wallpaper-clocks-live-earth.html)
* [NoobsLab](https://www.noobslab.com/2016/05/wallch-4-wallpaper-manager-live-clock.html)
* Youtuber [InfinitelyGalactic](https://www.youtube.com/watch?v=CnWsC4kIHn8)
* Article in greek magazine [Linux Inside](https://www.linuxinsider.gr/magazine/linux-inside-6-parallilos-programmatismos)

### Downloads

Wallch counts over 50k downloads at the [SourceForge repository](https://sourceforge.net/projects/wall-changer/), not including downloads from PPAs or the Ubuntu Software Center. :squirrel:

## License

See the [LICENSE](https://github.com/LeonVitanos/Wallch/blob/master/LICENSE) file for license rights and limitations (GPLv3).
