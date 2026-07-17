const config_module = require('./config')
const Redis = require("ioredis")

const RedisCli = new Redis({
    host: config_module.redis_host,
    port: config_module.redis_port,
    password: config_module.redis_password,
});

RedisCli.on("error",function(err){
    console.log("RedisCli connect error:", err.message);
    RedisCli.quit();
});

async function GetRedis(key) {
    try{
        const result = await RedisCli.get(key)
        if(result === null){
            console.log('result:','<'+result+'>', 'This key cannot be find...')
            return null
        }
        console.log('Result:','<'+result+'>','Get key success!...');
        return result
    }
    catch(error){
        console.log('GetRedis error is', error);
        return null
    }
}

async function QueryRedis(key) {
    try{
        const result = await RedisCli.exists(key)
        if(result === 0){
            console.log('result:<','<'+result+'>','This key is null...')
            return null
        }
        console.log('Result:','<'+result+'>','With this value!...');
        return result
    }
    catch(error){
        console.log('QueryRedis error is', error);
        return null
    }
}

async function SetRedisExpire(key,value,exptime) {
    try{
        await RedisCli.set(key,value,"EX",exptime)
        return true
    }
    catch(error){
        console.log('SetRedisExpire error is', error);
        return false;
    }
}

function Quit(){
    RedisCli.quit();
}

module.exports = {
    GetRedis,
    QueryRedis,
    SetRedisExpire,
    Quit,
}