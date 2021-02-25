<h1 align="center">SSTT</h1>

<p align="center">

<img src="https://img.shields.io/badge/made%20for-GTA%20SA--MP-blue" >

<img src="https://img.shields.io/badge/SA--MP-0.3.7--R1-blue" >

<img src="https://img.shields.io/badge/Server-Any-red">

<img src="https://img.shields.io/github/languages/top/qrlk/sstt">

<img src="https://img.shields.io/badge/dynamic/json?color=blueviolet&label=users%20%28active%29&query=result&url=http%3A%2F%2Fqrlk.me%2Fdev%2Fmoonloader%2Fusers_active.php%3Fscript%3Dsstt">

<img src="https://img.shields.io/badge/dynamic/json?color=blueviolet&label=users%20%28all%20time%29&query=result&url=http%3A%2F%2Fqrlk.me%2Fdev%2Fmoonloader%2Fusers_all.php%3Fscript%3Dsstt">

<img src="https://img.shields.io/date/1590958800?label=released" >

</p>

**SA:MP Speech-To-Text** is an **ASI plugin** that introduces voice input to **[gta samp](https://sa-mp.com/)**.

Recognizes only Russian, although you can change the backend url manually as you want (in code, not in json).

It requires **[SAMP 0.3.7-R1](http://files.sa-mp.com/sa-mp-0.3.7-install.exe)** and **[ASI Loader](https://www.gtagarage.com/mods/show.php?id=21709)**, depends on bass.dll, libcurl.dll и zlib1.dll (all included in [the release](https://github.com/qrlk/sstt/releases/latest)).

---

**The following description is in Russian, because it is the main language of the user base**.

# SSTT - SA:MP Speech-To-Text

**Описание:** Простой ASI плагин, который записывает по **удержанию клавиши** звук с микрофона, сохраняет в wav, распознаёт через инфраструктуру Яндекса и возвращает результат прямо в чат сампа.  

![](https://i.imgur.com/8gTul0K.png)

**Горячие клавиши:** R - говорить, P - /s [текст], N - /r [текст], J - /me [текст], L - /m [текст], B - /b [текст]

**Требования:** [SAMP 0.3.7-R1](http://files.sa-mp.com/sa-mp-0.3.7-install.exe) и [ASI Loader](https://www.gtagarage.com/mods/show.php?id=21709). А так же библиотеки bass.dll, libcurl.dll и zlib1.dll (две последние есть в архиве релиза).

P.S. Для пользователей из Украины нужен VPN.  
# Настройка горячих клавиш
В [версии от 15.07.2020](https://github.com/qrlk/sstt/releases/tag/v5%2C0) была добавлена возможность настроить свой набор фраз.  
Для этого нужно выйти из игры и отредактировать файл sstt.json:
```json
{
    "backend": 1,
    "header": "Content-Type: audio/x-wav",
    "presets": {
        "/b": {
            "key": "B",
            "text": "/b "
        },
        "/m": {
            "key": "L",
            "text": "/m "
        },
        "/me": {
            "key": "J",
            "text": "/me "
        },
        "/r": {
            "key": "N",
            "text": "/r "
        },
        "/s": {
            "key": "P",
            "text": "/s "
        },
        "prosto v chat": {
            "key": "R",
            "text": ""
        }
    },
    "url": "http://asr.yandex.net/asr_xml?uuid=12345678123456781234546112345678&disableAntimat=true&topic=general&lang=ru-RU&key=6372dda5-9674-4413-85ff-e9d0eb2f99a7"
}
```
Сейчас работает только смена пресетов по такому шаблону:
```json
"nazvanie shablona": {
    "key": "Odna Knopka",
    "text": "Prefix v chat"
}
```

## Ссылки
* [Тема на blasthack](https://blast.hk/threads/56447/)
* [Авторский обзор](http://www.youtube.com/watch?v=5SnM3AYhINk)
