RedditQuick - Lightech Unofficial Reddit Client
===============================================

Quickly read Reddit posts. This text-only client saves you significant amount of data, consume about 10MB-20MB of RAM at runtime as opposed to (200MB when viewed in Edge), thus save battery.

You can download this app on [Microsoft Store](https://www.microsoft.com/store/apps/9NG2S08KMLWK) or with the Store app ms-windows-store://pdp/?productid=9NG2S08KMLWK.

This client implements infinite scrolling that is missing from the Reddit website.

This application also illustrates the usage of our UWP utilities library [LUwpUtilities](https://github.com/light-tech/LUwpUtilities.git) to quickly build UWP application.
In particular, this app makes use of
 * `TaskHelper` to run code in background;
 * `HttpHelper` to fetch web content;
 * `IncrementalLoadingList` to provide infinite loading;
 * `XamlHelper`, `CustomPropertyBase` to build UI.
Thanks to LUwpUtilities, the app now compiles to 3.6MB, a fifth of the original XAML-version (20MB).
And there is no header dependency and out-of-heap-space to worry about.
This app (modulo LUwpUtilities) is implemented within 400 lines of code.

License
-------

 * Lightech Public License:
   You are free to use this library however you want as long as you give attribution to us. In particular, simply inform the user that your program uses RedditQuick at https://github.com/light-tech/RedditQuick.git in binary or source distribution of your program.