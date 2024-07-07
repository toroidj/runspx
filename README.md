 ------------------------------------------------------------------------------
runspi / runsph / runspha	Version 1.2 Copyright (c)2024 TORO
 ------------------------------------------------------------------------------

Susie Plug-in を使って画像の取得、書庫展開をしたり、Susie Plug-inの
動作検証を行ったりするためのコンソールアプリケーションです。

Susie Plug-in は、竹村嘉人 (たけちん)氏作の画像ローダ「Susie」
用の Plug-in で、様々な形式の画像ファイルを読み込めるようにする
ためのものです。
( http://www.digitalpad.co.jp/~takechin/ )


●動作環境
・対応/動作確認 OS

Windows 2003/XP 以降で動作します。

・レジストリ

レジストリは使用しません。


●実行方法

対象となる Susie Plug-in の種類によって対応するファイルを
実行してください。以下、runspx.exe は以下の名前に置き換えてください。

runspi.exe 拡張子 spi (x86)用
runsph.exe 拡張子 sph (x64)用
runspha.exe 拡張子 spha (ARM64)用

(1) Susie Plug-in の情報

runspx.exe (Plug-inのパス)

用意されている API の一覧を表示します。


(2) 画像の情報表示

runspx.exe (Plug-inのパス) (読み込む画像のパス)

指定された画像を Plug-in の GetPicture で読み込み、その画像の情報を
表示します。


(3) 画像の取得

runspx.exe (Plug-inのパス) (読み込む画像のパス) -b (展開先の画像のパス)

指定された画像を Plug-in の GetPicture で読み込み、BMP ファイルとして
保存します。


(4) 書庫の一覧表示

指定された画像を Plug-in の GetArchiveInfo で読み込み、書庫内の
ファイル一覧を表示します。

runspx.exe (Plug-inのパス) (読み込む書庫のパス)


(5) 書庫の全ファイル展開

指定された画像を Plug-in の GetArchiveInfo で読み込み、書庫内の
全ファイルを展開先に保存します。

runspx.exe (Plug-inのパス) (読み込む書庫のパス) [-j] -e (展開先)
-j はディレクトリ指定を無視します


(6) 書庫の指定ファイル展開

指定された画像を Plug-in の GetArchiveInfo で読み込み、書庫内の
指定ファイルを展開先に保存します。

runspx.exe (Plug-inのパス) (読み込む書庫のパス) -e (展開先) (展開ファイル名)


(7) 画像のエンコード

指定された画像を指定した拡張子の形式にエンコードしてファイルに
保存します。

runspx.exe (Plug-inのパス) (読み込む画像のパス) -bc (展開先の画像のパス) [-bp (エンコードに使用する Plug-inのパス)]


(8) Susie Plug-in のテスト

runspx.exe (Plug-inのパス) (読み込む画像・書庫のパス) -t [-p]
runspx.exe (Plug-inのパス) (読み込む画像・書庫のパス) -tz [-p]

全APIを一通り使用するテストを行います。

挙動についてコメントがある場合は「●コメント内容」を出力します。
「End test.」が表示されずに終了したときは、異常終了です。
	
書庫のテストやCreatePictureのテストを行ったときは、%temp%\runspx に
結果が残ります。

異常終了したとき、スタックトレース表示が可能であれば表示します。
表示できない場合は、主にスタックの破損か、スタックオーバフローと
思われます。
尚、デバッガを使用する場合は -p を指定することで
スタックトレース表示を行わず、デバッガに処理を渡せます。

-tz を指定したときは、0 バイトファイルを渡すテストを追加します。


(8)オプション一覧

-c		Plug-in の設定ダイアログを表示します。
-v		Plug-in の About ダイアログを表示します。
-b 保存先パス	GetPicture / GetPreview で得たビットマップをBMP
		ファイルとして保存します。
-bc 保存先パス	GetPicture / GetPreview で得たビットマップを
		CreatePicture で保存します。
-bp Pluginパス	CreatePicture を使う Susie Plug-in を指定します。
-r		GetPreview を GetPicture の代わりに使用します。
-t		Plug-in のテストを行います。
-tz		Plug-in のテストを行います。ファイズサイズが 0 の
		テストも行います。
-p		異常終了したとき、スタックトレースを行いません。
-e		GetFile による展開を行います。
-j		GetFile による展開時、サブディレクトリを無視します。
-srcdisk	入力ソースをファイルにします。
-srcmem		入力ソースをメモリにします。
-ansi		UNICODE 版API を使用しないでANSI 版API のみ使用します。


●最後に

・このソフトウェアはフリーソフトウェアです。著作権は「TORO」、
  「高橋 良和」にあります。

・runspi, runsph は、Visual Studio 2008、
  runspha は、Visual Studio 2019 を用いて構築しています。


●履歴
Version 1.2	2024/03/30
・GetFileのテストで、ソースがメモリの時の動作がおかしいのを修正

Version 1.1	2024/03/17
・書庫の指定ファイル展開の説明の誤記修正と、展開が終わらないことがあるのを修正

Version 1.0	2024/03/16
・初版

●著作権者／連絡先／一次配布先					TORO／高橋 良和
