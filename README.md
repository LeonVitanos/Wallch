# Wallch

This is my first project, in which i took my first steps of programming together with [Alexandros Solanos](https://github.com/hytromo), at the age of 15.

## Development Status: Wallch 5

Wallch 5 is currently under active development. This version represents a significant rewrite, primarily aimed at achieving broad compatibility across Linux, Windows, and macOS, as the methods for changing wallpapers have evolved and diverged significantly since v4 was originally developed.

> 🧹 **Milestone (June 14, 2025):** The project now compiles error- and warning-free on Linux, Windows, and macOS for both Qt 5.15.2 and modern Qt 6 versions.

The original Live Earth and Wikipedia Picture of the Day features are currently non-functional.

Below is a snapshot of the current compatibility status based on recent testing:

| OS/DE | Status | Change Image (Local) | Solid Color   | Style/Sizing | Notes                                                                 |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **macOS** |        |                      |               |              |                                                                       |
| macOS Monterey v12.7.6             | ➖     | ✅                    | ❌            | ❌           | compatibility on other macOS versions unknown. |
| **Windows** |        |                      |               |              |                                                                       |
| Windows 11                         | ➖     | ✅                    | ➖            | ✅           | Currently changes wallpaper only on the active desktop. Full multi-desktop support is planned. |
| **Linux** |        |                      |               |              | *Specific DE/OS versions heavily impact compatibility.* |
| Ubuntu 22.04 and 25.04 (GNOME)        | ✅     | ✅                    | ✅            | ✅           |                                                                       |
| Linux Mint 21.1 XFCE 4.16.4        | ➖     | ✅                    | ❌            | ❌           |                                |
| Fedora 37 LXDE PCManFM 1.3.2       | ❌     | ❌                    | ❌            | ❌           |           |
| Lubuntu 18.04 LXDE PCManFM 1.2.5   | ❌     | ❌                    | ❌            | ❌           |       |
| Kubuntu 22.04 KDE Plasma 5.24.7    | ❌     | ❌                    | ❌            | ❌           |  |
| Lubuntu 22.04 LXQt 0.17            | ➖     | ✅                    | ❌            | ❌           |                      |

### About

The development was active from 2010 until 2015 and was stopped mainly because we didn't admire the code we've written while learning to program and also wanted to move to other projects. 

![alt text](https://i.imgur.com/j6vWRv1.jpg)

It served as a wallpaper changer from images stored on the hard drive, but what made it popular to the Linux community, was some unique features, like the [Live Earth Wallpaper](https://www.die.net/earth/rectangular.html?zoom=2) and [Wikipedia's Picture Of The Day](https://en.wikipedia.org/wiki/Wikipedia:Picture_of_the_day). In order for those features to work, we had a Raspberry Pi powered on all the time and running scripts which were downloading, processing (merging live weather images, downsizing, etc...) and uploading the corresponding images 24/7.

With the help of an [OMG! Ubuntu! article](https://www.omgubuntu.co.uk/2012/03/spare-10-minutes-help-translate-test-wallpaper-app-wallch), Wallch has been translated to more than 7 languages!

### Reviews

* [OMG! Ubuntu!](https://www.omgubuntu.co.uk/2011/08/wallch-wallpaper-changer-adds-unity-features)
* [Web Upd8](http://www.webupd8.org/2014/06/wallch-use-wallpaper-clocks-live-earth.html)
* [NoobsLab](https://www.noobslab.com/2016/05/wallch-4-wallpaper-manager-live-clock.html)
* Youtuber [InfinitelyGalactic](https://www.youtube.com/watch?v=CnWsC4kIHn8)
* Article in greek magazine [Linux Inside](https://www.linuxinsider.gr/magazine/linux-inside-6-parallilos-programmatismos)

### Downloads

Wallch counts 50k+ downloads at the [sourceforge repository](https://sourceforge.net/projects/wall-changer/), not counting downloads from the ppas or from the Ubuntu Software Center (where most people download apps), which is pretty neat! :squirrel:

![alt text](https://i.imgur.com/xhW4KOv.png)

## License
See the [LICENSE](https://github.com/LeonVitanos/Wallch/blob/master/LICENSE) file for license rights and limitations (GPLv3).
