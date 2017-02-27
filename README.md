# Felis

一个简单、高效的关键词匹配服务

```
                                    
_|_|_|_|          _|  _|            
_|        _|_|    _|        _|_|_|  
_|_|_|  _|_|_|_|  _|  _|  _|_|      
_|      _|        _|  _|      _|_|  
_|        _|_|_|  _|  _|  _|_|_|    

```

## 编译安装

```
$ git clone --recursive https://github.com/imaben/felis.git
$ cd felis/src
$ make
$ ./build/felis --help
```

## Usage

```

Usage: felis [options]
Options:
-h <host>        listen host
-p <port>        port number
-d               daemon mode
-t <threads>     threads count
-o <timeout>     http request timeout second
-l <logfile>     log file
-m <debug|notice|warn|error>
-H               show help

```


## 函数说明

### 获取词典列表

#### Requesst

```
GET http://felishost/dict
```

#### Response

```
[
    {
        "name": "dict1",
        "count": 100
    },
    {
        "name": "dict2",
        "count": 1000
    }
]
```

### 添加词典

#### Requesst

```
POST http://felishost/dict
{
	"name": "dictname"
}
```

#### Response

```
{
	"result": true
}
```

### 添加关键词

#### Request

```
POST http://felishohst/dict/{dictname}
{
	"word": "hello",
	"ext": "extention string"
}
```
#### Response

```
{
	"result": true
}
```

### 匹配

#### Request

```
POST http://felishost/match/{dictname}
{
	"content": "hello world"
}
```

#### Response

```
[
	{
		"word": "hello",
		"ext": null
	}
]
```
