/****************************************************************/
/*      Novation Launchpad Mini Mapping                         */
/*      For Mixxx version 1.12                                  */
/*      Author: marczis based on zestoi's work                  */
/****************************************************************/

//Common helpers
colorCode = function()
{
    return {
        black: 4,

        lo_red: 1 + 4,
        mi_red: 2 + 4,
        hi_red: 3 + 4,
        lo_green: 16 + 4,
        mi_green: 32 + 4,
        hi_green: 48 + 4,
        lo_amber: 17 + 4,
        mi_amber: 34 + 4,
        hi_amber: 51 + 4,
        hi_orange: 35 + 4,
        lo_orange: 18 + 4,
        hi_yellow: 50 + 4,
        lo_yellow: 33 + 4,

        /*
        flash_lo_red: 1,
        flash_mi_red: 2,
        flash_hi_red: 3,
        flash_lo_green: 16,
        flash_mi_green: 32,
        flash_hi_green: 48,
        flash_lo_amber: 17,
        flash_mi_amber: 34,
        flash_hi_amber: 51,
        flash_hi_orange: 35,
        flash_lo_orange: 18,
        flash_hi_yellow: 50,
        flash_lo_yellow: 33
        TODO fix these*/
    }
};
//Define one Key
Key = Object;
Key.prototype.color = colorCode("black");
Key.prototype.x = -1;
Key.prototype.y = -1;
Key.prototype.page = -1;
Key.prototype.pressed = false;

Key.prototype.init = function(x,y, page)
{
    this.x = x;
    this.y = y;
    this.page = page;
    //print("Key created");
}

Key.prototype.setColor = function(color)
{
    //First line is special
    this.color = colorCode()[color];
    this.draw();
};

Key.prototype.draw = function()
{
    if ( this.page != NLM.page ) return;
    if ( this.y == 8 ) {
        midi.sendShortMsg(0xb0, this.x + 0x68, this.color);
        return;
    }
    midi.sendShortMsg(0x90, this.x+this.y*16, this.color);
    //midi.sendShortMsg(0xb0, 0x0, 0x28); //Enable buffer cycling
}

Key.prototype.onPush = function()
{
}

Key.prototype.onRelease = function()
{
}

Key.prototype.callback = function()
{
    if (this.pressed) {
        this.onPush();
    } else {
        this.onRelease();
    }
}

function PushKey(colordef, colorpush) {
    var that = new Key;

    that.setColor(colordef);

    that.colordef = colordef;
    that.colorpush = colorpush;

    that.onPush = function()
    {
        this.setColor(this.colorpush);
    }

    that.onRelease = function()
    {
        this.setColor(this.colordef);
    }

    return that;
}

function PageSelectKey() {
    var that = new Key;

    that.onPush = function()
    {
        NLM.btns[NLM.page][8][NLM.page].setColor("black");
        NLM.page = this.y;
        NLM.btns[NLM.page][8][NLM.page].setColor("hi_amber");
        NLM.drawPage();
    }
    return that;
}

function ShiftKey()
{
    var that = PushKey("lo_green", "hi_yellow");

    that.onPushOrig = that.onPush;
    that.onPush = function()
    {
        NLM.shiftstate = this.pressed;
        this.onPushOrig();
    }

    that.onReleaseOrig = that.onRelease;
    that.onRelease = function()
    {
        NLM.shiftstate = this.pressed;
        this.onReleaseOrig();
    }

    return that;
}

function HotCueKey(deck, hotcue)
{
    var that = new Key();
    that.deck = deck;
    that.hotcue = hotcue;

    that.group = "[Channel" + deck + "]";
    that.ctrl_act = "hotcue_" + hotcue + "_activate";
    that.ctrl_del = "hotcue_" + hotcue + "_clear";
    that.state   = "hotcue_" + hotcue + "_enabled";

    that.setled = function() {
        if (this.pressed) {
            this.setColor("hi_amber");
        } else if (engine.getValue(this.group, this.state) == 1) {
            this.setColor("lo_green");
        } else {
            this.setColor("lo_red");
        }
    }

    that.conEvent = function() {
        engine.connectControl(this.group, this.state, this.setled);
    }

    that.setled();
    that.conEvent();

    that.callback = function() {
        if (NLM.shiftstate) {
            ctrl = this.ctrl_del;
        } else {
            ctrl = this.ctrl_act;
        }

        if (this.pressed) {
            engine.setValue(this.group, ctrl, 1);
        } else {
            engine.setValue(this.group, ctrl, 0);
        }

        this.setled();
    }

    return that;
}

function PlayKey(deck)
{
    var that = new Key();
    that.group = "[Channel" + deck + "]";
    that.ctrl  = "play";
    that.state = "play_indicator";

    that.setled = function() {
        if (this.pressed) {
            this.setColor("hi_amber");
        } else if (engine.getValue(this.group, this.state) == 1) {
            this.setColor("hi_green");
        } else {
            this.setColor("hi_yellow");
        }
    }

    that.conEvent = function() {
        engine.connectControl(this.group, this.state, this.setled);
    }

    that.setled();
    that.conEvent();

    that.onPush = function()
    {
        engine.setValue(this.group, this.ctrl, engine.getValue(this.group, this.ctrl) == 1 ? 0 : 1);
        this.setled();
    }

    return that;
}

function LoopKey(deck, loop)
{
    var that = new Key();

    that.group = "[Channel" + deck + "]";
    that.ctrl0 = "beatloop_" + loop + "_toggle";
    that.ctrl1 = "beatlooproll_" + loop + "_activate";
    that.state = "beatloop_" + loop + "_enabled";
    that.setColor("hi_yellow");

    if (LoopKey.keys == undefined) {
        LoopKey.keys = new Array;
        LoopKey.mode = 0;
    }

    LoopKey.setMode = function(mode)
    {
        LoopKey.mode = mode;
        if (mode == 1) {
            LoopKey.keys.forEach(function(e) { e.setColor("hi_orange");} );
        }
        if (mode == 0) {
            LoopKey.keys.forEach(function(e) { e.setColor("hi_yellow");} );
        }
    }

    that.callback = function()
    {
        if (LoopKey.mode == 0) {
             if (this.pressed) {
                engine.setValue(this.group, this.ctrl0, 1);
                this.setColor("hi_green");
            } else {
                if ( engine.getValue(this.group, this.state) == 1) {
                    engine.setValue(this.group, this.ctrl0, 1);
                }
                this.setColor("hi_yellow");
            }
        } else {
            if (this.pressed) {
                engine.setValue(this.group, this.ctrl1, 1);
                this.setColor("hi_green");
            } else {
                engine.setValue(this.group, this.ctrl1, 0);
                this.setColor("hi_orange");
            }
        }
    }

    LoopKey.keys.push(that);
    return that;
}

LoopCallback = function(key, deck, loop)
{
    this.key = key;
}

LoopModeCallback = function(key)
{
    this.key = key;
    this.key.setColor("lo_yellow");
}

LoopModeCallback.prototype.f = function()
{
    if (this.key.pressed) {
        if (LoopCallback.mode == 0) {
            LoopCallback.setMode(1);
            this.key.setColor("lo_orange");
        } else {
            LoopCallback.setMode(0);
            this.key.setColor("lo_yellow");
        }
    }
}

//Define the controller

NLM = new Controller();
NLM.init = function()
{
        NLM.page = 0;
        NLM.shiftstate = false;
        NLM.numofdecks = engine.getValue("[Master]", "num_decks");

        //Init hw
        midi.sendShortMsg(0xb0, 0x0, 0x0);
        //midi.sendShortMsg(0xb0, 0x0, 0x28); //Enable buffer cycling <-- Figure out whats wrong with this

        // select buffer 0
        midi.sendShortMsg(0xb0, 0x68, 3);
        //midi.sendShortMsg(0xb0, 0x0, 0x31);
        //print("=============================");
        //Setup btnstate which is for phy. state
        NLM.btns = new Array();
        for ( page = 0; page < 8 ; page++ ) {
            NLM.btns[page] = new Array();
            for ( x = 0 ; x < 9 ; x++ ) {
                NLM.btns[page][x] = new Array();
                for ( y = 0 ; y < 9 ; y++ ) {
                    var tmp = new Key;
                    if (x == 8) {
                        tmp = PageSelectKey();
                    }

                    if (y == 8 && x == 7) {
                        tmp = ShiftKey();
                    }

                    tmp.init(x,y, page);
                    NLM.btns[page][x][y] = tmp;
//                    NLM.setColor(x, y, "hi_yellow");
                }
            }
        }
        //Set default page led
        NLM.btns[NLM.page][8][0].setColor("hi_amber");

        //TODO make this dynamic based on numofdecks !
        //Set ChX CueButtons
        for ( deck = 1; deck <= NLM.numofdecks; deck++ ) {
            for ( hc = 1 ; hc < 9 ; hc++ ) {
                x = hc-1;
                y = (deck-1)*2+1;
                NLM.btns[0][x][y] = HotCueKey(deck, hc);
                NLM.btns[0][x][y].init(x,y, 0);
            }
        }

        for ( deck = 1; deck <= NLM.numofdecks; deck++ ) {
            y = (deck-1)*2;
            //Set Chx PlayButton
            NLM.btns[0][0][y] = PlayKey(deck);
            NLM.btns[0][0][y].init(0, y, 0);
            //Set Chx LoopButtons
            NLM.btns[0][2][y] = LoopKey(deck, "0.0625");
            NLM.btns[0][2][y].init(0, y, 2);
            NLM.btns[0][3][y] = LoopKey(deck, "0.125");
            NLM.btns[0][3][y].init(0, y, 3);
            NLM.btns[0][4][y] = LoopKey(deck, "0.25");
            NLM.btns[0][4][y].init(0, y, 4);
            NLM.btns[0][5][y] = LoopKey(deck, "0.5");
            NLM.btns[0][5][y].init(0, y, 5);
            NLM.btns[0][6][y] = LoopKey(deck, "1");
            NLM.btns[0][6][y].init(0, y, 6);
            NLM.btns[0][7][y] = LoopKey(deck, "2");
            NLM.btns[0][7][y].init(0, y, 7);
        }

        //- NLM.btns[0][2][8].callback = new LoopModeCallback(NLM.btns[0][2][8]);

        this.drawPage();
};


NLM.shutdown = function()
{

};

NLM.incomingData = function(channel, control, value, status, group)
{
        //print("Incoming data");
        //print("cha: " + channel);
        //print("con: " + control);
        //print("val: " + value);
        //print("sta: " + status);
        //print("grp: " + group);

        //Just to make life easier
        var pressed = (value == 127);
        //Translate midi btn into index
        var y = Math.floor(control / 16);
        var x = control - y * 16;
        if ( y == 6 && x > 8 ) {
            y = 8;
            x -= 8;
        }
        if ( y == 6 && x == 8 && status == 176 ) {
            y = 8; x = 0;
        }

        print( "COO: " + y + ":" + x);
        NLM.btns[NLM.page][x][y].pressed = pressed;
        NLM.btns[NLM.page][x][y].callback();
};

NLM.drawPage = function() {
    for ( x = 0 ; x < 9 ; x++ ) {
        for ( y = 0 ; y < 9 ; y++ ) {
            NLM.btns[NLM.page][x][y].draw();
        }
    }
}

