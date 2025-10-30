import { AnimatorResult } from '@ohos.animator';
import Space from '../../../../.preview/cache/.default/entry/src/main/ets/component/SpaceComponent';

@Component
export default struct BPMControllerComponent {
  @Link bat: number;
  @Link BPM: number;
  @Consume animateManager?: AnimatorResult; // 动画管理对象
  @State quarter: number = 0; // 动画刻
  @State startHover: boolean = false; // 按钮是否选中
  minBPM: number = 0; // 最小BPM
  maxBPM: number = 240; // 最大BPM

  /**
   * 按钮按键事件
   */
  startTouch = (event: TouchEvent) => {
    switch (event.type) {
      case TouchType.Down:
        this.startHover = true;
        break;
      case TouchType.Up:
        this.startHover = false;
        break;
      default:
        break;
    }
  }

  start() {

  }

  /**
   * 更新BPM
   * @param type
   */
  updateBPM(type: "add" | "sub") {
    if (type === "add") {
      this.BPM += 5;
    }
    if (type === "sub") {
      if (this.BPM > 0) {
        this.BPM -= 5;
      }
    }
  }

  /**
   * 更新节拍
   * @param type
   */
  upDateBat(type: "add" | "sub") {
    if (this.animateManager) {
      this.animateManager.play()
    }
    if (type === "add") {
      this.bat++;
    }
    if (type === "sub") {
      if (this.bat > 0) {
        this.bat--;
      }
    }
  }

  build() {
    Column({ space: 20 }) {
      Text($r("app.string.title"))
        .fontColor($r("app.color.fontColor"))
        .fontWeight(FontWeight.Medium)

      Row({ space: 30 }) {
        Button() {
          Text(`-`)
            .buttonText()
        }
        .width(50)
        .height(50)
        .backgroundColor($r("app.color.circleFill"))
        .type(ButtonType.Circle)
        .onClick(() => this.upDateBat("sub"))

        Text(this.bat.toFixed(0))
          .fontSize(50)
          .fontColor($r("app.color.fontColor"))
        Button() {
          Text(`+`)
            .buttonText()
        }
        .height(50)
        .width(50)
        .backgroundColor($r("app.color.circleFill"))
        .type(ButtonType.Circle)
        .onClick(() => this.upDateBat("add"))

      }

      Row({ space: 10 }) {
        Text(`${this.minBPM}`)
          .fontColor($r("app.color.fontColor"))
        Slider({ value: this.BPM, min: this.minBPM, max: this.maxBPM, step: 1 })
          .onChange(value => {
            this.BPM = ~~value;
          })
          .layoutWeight(1)

        Text(`${this.maxBPM}`)
          .fontColor($r("app.color.fontColor"))
      }
      .width(300)

      Row({ space: 20 }) {
        Button() {
          Text('-5')
            .buttonText()
        }
        .onClick(() => this.updateBPM("sub"))
        .backgroundColor($r("app.color.circleFill"))
        .width(50)
        .height(50)
        .type(ButtonType.Circle)

        Button() {
          Text('+5')
            .buttonText()
        }
        .onClick(() => this.updateBPM("add"))
        .backgroundColor($r("app.color.circleFill"))
        .width(50)
        .height(50)
        .type(ButtonType.Circle)

      }

      Button($r("app.string.start"))
        .onClick(() => this.start())
        .backgroundColor(Color.Transparent)
        .linearGradient({
          angle: 90,
          direction: GradientDirection.Top,
          colors: this.startHover ? [["rgba(60,60,83,0.3)", 1], ["rgba(20,20,184,0.3)", 0]] : [["rgb(60,60,83)", 0], ["rgb(20,20,184)", 1]]
        })
        .stateEffect(false)
        .onTouch(this.startTouch)
        .width(300)
    }
    .width("100%")
    .padding(10)
    .backgroundColor(Color.Transparent)
  }
}


@Extend(Text)
function buttonText() {
  .fontColor($r("app.color.fontColor"))
  .fontSize(24)
  .fontWeight(FontWeight.Medium)
}