# ics-exp-lan
NW experiment

## 準備
1. ics-exp-lan/share/Source以下に新しいディレクトリを作成する
2. 作成したディレクトリにこのリポジトリをクローンする  
   (gitがインストールされてなければzipをダウンロードして解凍する)

## コンパイル方法
### 両方:
* make

### throughput.out:
* make th

### ping.out:
* make pi

## 実行方法
### throughput.out:
1. node2~5で./throughputN.out N sを実行する (Nは各ノードの番号)
2. node1で./throughput1.out 1 cを実行する

### ping.out:
1. node2~5で./pingN.out N sを実行する (Nは各ノードの番号)
2. node1で./ping1.out 1 cを実行する
